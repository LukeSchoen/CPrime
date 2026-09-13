/* Lambda expressions have distinct closure types, including when an
   expression is first replayed for deduction and later for code emission. */
typedef struct LambdaCapture {
  int name, by_reference;
  CType type;
  Sym *source;
} LambdaCapture;

typedef struct LambdaBodyDeclaration {
  int name, scope_depth, index;
} LambdaBodyDeclaration;

typedef struct LambdaExpression {
  int marker, owner, class_tok, body_owner_tok, count, default_capture, mutable_call;
  int return_seen, explicit_return, outer_scope, enclosing_class_tok;
  int return_deduction_kind, generic, generic_param_count;
  int generic_param_toks[16], generic_param_is_pack[16];
  int body_declaration_count;
  LambdaBodyDeclaration body_declarations[256];
  CType return_pattern;
  CType return_type;
  Sym *outer_this;
  LambdaCapture captures[64];
  int static_count, static_names[64];
  Sym *static_symbols[64];
  struct LambdaExpression *next, *parent_probe;
} LambdaExpression;

static int lambda_capture_field_tok(LambdaExpression *lambda, int index)
{
  return lambda->captures[index].source == lambda->outer_this
      ? tok_alloc_const("__cpc_captured_this") : lambda->captures[index].name;
}

static LambdaExpression *lambda_expressions, *lambda_probe;
static unsigned lambda_expression_counter;
static int lambda_body_type_owner(int class_tok)
{
  LambdaExpression *lambda;
  for (lambda = lambda_expressions; lambda; lambda = lambda->next)
    if (lambda->class_tok == class_tok)
      return lambda->body_owner_tok;
  return 0;
}
static void probe_lambda_body(LambdaExpression *lambda, TokenString *parameters,
                              TokenString *body);

static void free_lambda_expressions(void)
{
  while (lambda_expressions)
  {
    LambdaExpression *next_lambda = lambda_expressions->next;
    cprime_free(lambda_expressions);
    lambda_expressions = next_lambda;
  }
  lambda_probe = NULL;
  lambda_expression_counter = 0;
}

static int is_cpp_lambda_marker(int token)
{
  return token >= TOK_UIDENT && token < SYM_FIRST_ANOM
         && !strncmp(get_tok_str(token, NULL), "__cpc_lambda_expression_", 24);
}

static int new_lambda_marker(void)
{
  char name[80];
  snprintf(name, sizeof(name), "__cpc_lambda_expression_%u",
           ++lambda_expression_counter);
  return tok_alloc_const(name);
}

static int new_lambda_generic_param(LambdaExpression *lambda)
{
  char name[96];
  int token;

  if (lambda->generic_param_count
      >= (int)(sizeof(lambda->generic_param_toks)
               / sizeof(lambda->generic_param_toks[0])))
    cprime_error("too many generic lambda parameters");
  snprintf(name, sizeof(name), "__cpc_lambda_auto_%d_%d",
           lambda->marker, lambda->generic_param_count);
  token = tok_alloc_const(name);
  lambda->generic_param_toks[lambda->generic_param_count++] = token;
  lambda->generic = 1;
  return token;
}

static void lambda_note_body_declaration(LambdaExpression *lambda, int name,
                                         int scope_depth, int index)
{
  int i;
  if (name < TOK_UIDENT)
    return;
  for (i = 0; i < lambda->body_declaration_count; ++i)
    if (lambda->body_declarations[i].name == name
        && lambda->body_declarations[i].scope_depth == scope_depth)
      return;
  if (lambda->body_declaration_count
      == (int)(sizeof(lambda->body_declarations)
               / sizeof(lambda->body_declarations[0])))
    cprime_error("too many local names in generic lambda");
  lambda->body_declarations[lambda->body_declaration_count].name = name;
  lambda->body_declarations[lambda->body_declaration_count].scope_depth =
    scope_depth;
  lambda->body_declarations[lambda->body_declaration_count].index = index;
  ++lambda->body_declaration_count;
}

static int lambda_body_name_is_local(LambdaExpression *lambda, int name,
                                     int scope_depth, int index)
{
  int i;
  for (i = lambda->body_declaration_count - 1; i >= 0; --i)
    if (lambda->body_declarations[i].name == name
        && lambda->body_declarations[i].scope_depth <= scope_depth
        && lambda->body_declarations[i].index <= index)
      return 1;
  return 0;
}

static int lambda_symbol_is_outer_automatic(LambdaExpression *lambda,
                                            Sym *symbol)
{
  return symbol && symbol->sym_scope
      && symbol->sym_scope <= lambda->outer_scope
      && !(symbol->type.t & (VT_STATIC | VT_EXTERN | VT_TYPEDEF))
      && (symbol->type.t & VT_BTYPE) != VT_FUNC
      && !IS_ENUM_VAL(symbol->type.t);
}

static int lambda_is_enclosing_member(int class_tok, int name)
{
  CType class_type;
  Sym *class_sym;
  int offset;
  if (!class_tok || name < TOK_UIDENT)
    return 0;
  class_sym = struct_find(class_tok);
  if (!class_sym)
    return 0;
  class_type.t = class_sym->type.t;
  class_type.ref = class_sym;
  {
    Sym *member = find_field_try(&class_type, name, &offset);
    if (member && !(member->type.t & VT_TYPEDEF)
        && !IS_ENUM_VAL(member->type.t))
      return 1;
  }
  {
    Sym *member = find_static_member_try(&class_type, name, NULL);
    return member && !(member->type.t & VT_TYPEDEF);
  }
}

/* `close` indexes the `]` that ends a bracketed list a saved body may spell as
   a lambda introducer.  The same bracket is a GNU array designator when the
   initializer clause continues with further designators and then `=`, as in
   `{ [e] = 0 }` or `{ [1][2] = 0 }`.  Direct parses apply that rule in the
   initializer code; a replayed template body only knows the lambda marker, so
   the shape has to be recognized here as well. */
static int saved_cpp_bracket_opens_designator(const TokenString *expression,
                                              int close)
{
  int len = expression->len;
  int index = close + 1;

  for (;;)
  {
    index = tok_str_next_non_linenum_index(expression->str, len, index);
    if (index >= len)
      return 0;
    if (expression->str[index] == '.')
    {
      index = tok_str_next_non_linenum_index(expression->str, len, index + 1);
      if (index >= len || expression->str[index] < TOK_UIDENT)
        return 0;
      ++index;
      continue;
    }
    if (expression->str[index] == '[')
    {
      int depth = 1;
      for (++index; index < len;)
      {
        int t = expression->str[index];
        if (TOK_HAS_VALUE(t))
        {
          index += 1 + tok_str_value_extra_words(expression->str, len, index);
          continue;
        }
        ++index;
        if (t == '[')
          ++depth;
        else if (t == ']' && --depth == 0)
          break;
      }
      if (depth)
        return 0;
      continue;
    }
    return expression->str[index] == '=';
  }
}

static void prepare_saved_cpp_lambdas(TokenString *expression)
{
  int i, previous = 0;
  if (!is_cpp_translation_unit() || !expression)
    return;
  for (i = 0; i < expression->len; ++i)
  {
    int t = expression->str[i];
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(expression->str, expression->len, i);
      if (t != TOK_LINENUM) previous = t;
      continue;
    }
    if (t == '[' && (!previous || previous == '=' || previous == '('
                    || previous == ',' || previous == TOK_RETURN
                    || previous == '{' || previous == '?' || previous == ':'))
    {
      int j = i + 1;
      for (; j < expression->len; ++j)
      {
        int capture = expression->str[j];
        if (capture == TOK_LINENUM) { ++j; continue; }
        if (capture == ']') break;
        if (TOK_HAS_VALUE(capture))
        {
          j += tok_str_value_extra_words(expression->str, expression->len, j);
          continue;
        }
        if (capture < TOK_UIDENT && capture != '&' && capture != '='
            && capture != ',') break;
      }
      if (j < expression->len && expression->str[j] == ']'
          && !saved_cpp_bracket_opens_designator(expression, j))
        expression->str[i] = new_lambda_marker();
    }
    previous = t;
  }
}

/* A name in an unevaluated operand does not odr-use an enclosing automatic
   object and therefore does not need a capture.  The deferred closure body is
   compiled away from the lexical frame, so fold the exact size/alignment query
   before capture analysis and body replay. */
static TokenString *fold_lambda_unevaluated_outer_sizes(
    LambdaExpression *lambda, TokenString *body)
{
  TokenString *result = tok_str_alloc();
  int i, changed = 0;

  for (i = 0; i < body->len; ++i)
  {
    int token = body->str[i];
    int open, name_index, close, size, alignment;
    Sym *symbol;
    CValue value;

    if (token == TOK_EOF)
      break;
    if (TOK_HAS_VALUE(token))
    {
      tok_str_add_record(result, body->str, body->len, &i);
      continue;
    }
    if ((token != TOK_SIZEOF && token != TOK_ALIGNOF1
         && token != TOK_ALIGNOF2 && token != TOK_ALIGNOF3
         && token != TOK_ALIGNOF4)
        || i + 1 >= body->len)
      {
        tok_str_add(result, token);
        continue;
      }
    open = i + 1;
    while (open < body->len && body->str[open] == TOK_LINENUM)
      open += 1 + tok_str_value_extra_words(body->str, body->len, open);
    if (open >= body->len || body->str[open] != '(')
    {
      tok_str_add(result, token);
      continue;
    }
    name_index = open + 1;
    while (name_index < body->len && body->str[name_index] == TOK_LINENUM)
      name_index += 1 + tok_str_value_extra_words(body->str, body->len,
                                                  name_index);
    if (name_index >= body->len || body->str[name_index] < TOK_UIDENT)
    {
      tok_str_add(result, token);
      continue;
    }
    close = name_index + 1;
    while (close < body->len && body->str[close] == TOK_LINENUM)
      close += 1 + tok_str_value_extra_words(body->str, body->len, close);
    if (close >= body->len || body->str[close] != ')')
    {
      tok_str_add(result, token);
      continue;
    }
    symbol = sym_find(body->str[name_index]);
    if (!symbol)
    {
      tok_str_add(result, token);
      continue;
    }
    if (!lambda_symbol_is_outer_automatic(lambda, symbol)
        || is_reference_type(&symbol->type)
        || (symbol->type.t & VT_VLA)
        || (symbol->type.t & VT_BTYPE) == VT_FUNC
        || (symbol->type.t & VT_BTYPE) == VT_VOID)
    {
      tok_str_add(result, token);
      continue;
    }
    size = type_size(&symbol->type, &alignment);
    if (size < 0)
    {
      tok_str_add(result, token);
      continue;
    }
    if (symbol->a.aligned)
      alignment = 1 << (symbol->a.aligned - 1);
    if (token == TOK_SIZEOF)
      value.i = size;
    else
      value.i = alignment;
    tok_str_add2(result, TOK_CULLONG, &value);
    changed = 1;
    i = close;
  }
  if (!changed)
  {
    tok_str_free(result);
    return body;
  }
  tok_str_add(result, TOK_EOF);
  return result;
}

static LambdaCapture *add_lambda_capture(LambdaExpression *lambda,
                                         Sym *source, int by_reference)
{
  LambdaCapture *capture;
  int i;
  for (i = 0; i < lambda->count; ++i)
    if (lambda->captures[i].source == source)
      return &lambda->captures[i];
  if (lambda->count == 64)
    cprime_error("too many lambda captures");
  /* Copy-capturing a reference to a function keeps a function reference;
     functions cannot become closure data members by value. */
  if (is_reference_type(&source->type)
      && (pointed_type(&source->type)->t & VT_BTYPE) == VT_FUNC)
    by_reference = 1;
  capture = &lambda->captures[lambda->count++];
  capture->name = source->v & ~SYM_FIELD;
  capture->source = source;
  capture->by_reference = by_reference;
  capture->type = source->type;
  if (is_reference_type(&capture->type))
    capture->type = *pointed_type(&capture->type);
  capture->type.t &= ~VT_STORAGE;
  if (by_reference)
  {
    mk_pointer(&capture->type);
    capture->type.t |= VT_REFERENCE;
  }
  if ((capture->type.t & VT_BTYPE) == VT_PTR)
    capture->type.ref = template_stable_pointer_ref(&capture->type);
  return capture;
}

static Sym *find_lambda_static_symbol(int name)
{
  LambdaExpression *lambda;
  int i;
  for (lambda = lambda_expressions; lambda; lambda = lambda->next)
    if (lambda->class_tok == active_member_class_tok)
      for (i = 0; i < lambda->static_count; ++i)
        if (lambda->static_names[i] == name) return lambda->static_symbols[i];
  return NULL;
}

static void lambda_note_static(LambdaExpression *lambda, Sym *source)
{
  Sym *alias;
  int i, name = source->v & ~SYM_FIELD;
  for (i = 0; i < lambda->static_count; ++i)
    if (lambda->static_names[i] == name) break;
  if (i == lambda->static_count) {
    char alias_name[96];
    if (i >= 64) cprime_error("too many static names used in lambda");
    snprintf(alias_name, sizeof(alias_name), "__cpc_lambda_static_%d_%d", lambda->class_tok, name);
    alias = global_identifier_push(tok_alloc_const(alias_name), source->type.t, source->c);
    lambda->static_names[i] = name;
    lambda->static_symbols[i] = alias;
    ++lambda->static_count;
  } else alias = lambda->static_symbols[i];
  alias->type = source->type;
  if ((alias->type.t & VT_BTYPE) == VT_PTR)
    alias->type.ref = template_stable_pointer_ref(&alias->type);
  alias->r = source->r;
  alias->c = source->c;
  alias->a = source->a;
  alias->const_value = source->const_value;
}

static Sym *lambda_capture_symbol(Sym *symbol)
{
  LambdaCapture *capture;
  CType visible_type;
  /* The deferred closure body is compiled outside the lexical frame, so an
     unevaluated mention still needs a synthetic capture to keep the name
     resolvable there. */
  if (cpp_unevaluated_expression_depth > 0 && lambda_probe
      && lambda_symbol_is_outer_automatic(lambda_probe, symbol))
  {
    add_lambda_capture(lambda_probe, symbol, 0);
    return symbol;
  }
  if (lambda_probe && symbol && symbol->sym_scope
      && symbol->sym_scope <= lambda_probe->outer_scope
      && (symbol->type.t & VT_STATIC)) {
    lambda_note_static(lambda_probe, symbol);
    return symbol;
  }
  if (!lambda_probe || !symbol || !symbol->sym_scope
      || symbol->sym_scope > lambda_probe->outer_scope
      || (symbol->type.t & (VT_STATIC | VT_EXTERN | VT_TYPEDEF))
      || (symbol->type.t & VT_BTYPE) == VT_FUNC || IS_ENUM_VAL(symbol->type.t))
    return symbol;
  if (!lambda_probe->default_capture)
    cprime_error("'%s' is not captured by the lambda",
                 get_tok_str(symbol->v & ~SYM_FIELD, NULL));
  capture = add_lambda_capture(lambda_probe, symbol,
                               lambda_probe->default_capture == '&'
                               && symbol != lambda_probe->outer_this);
  visible_type = capture->type;
  if (!capture->by_reference && !lambda_probe->mutable_call)
    visible_type.t |= VT_CONSTANT;
  return sym_push(capture->name, &visible_type, VT_LOCAL | VT_LVAL, 0);
}

static CType deduce_placeholder_return_pattern(CType *pattern, CType *actual)
{
  CType result = *actual;
  if ((pattern->t & VT_BTYPE) == VT_PTR && !is_reference_type(pattern))
  {
    if ((actual->t & VT_BTYPE) != VT_PTR || is_reference_type(actual))
      cprime_error("return expression does not match auto pointer declarator");
    result = deduce_placeholder_return_pattern(pointed_type(pattern),
                                               pointed_type(actual));
    mk_pointer(&result);
    result.t |= pattern->t & (VT_CONSTANT | VT_VOLATILE);
  }
  else
    result.t |= pattern->t & (VT_CONSTANT | VT_VOLATILE);
  return result;
}

static CType deduce_placeholder_return_expression(LambdaExpression *deduction)
{
  CType actual = vtop->type, pattern = deduction->return_pattern, result;
  int lvalue = !(vtop->r & VT_CXX_PRVALUE)
      && !(actual.t & VT_RVALUE_REFERENCE)
      && ((vtop->r & VT_LVAL) || (actual.t & VT_ARRAY)
          || (actual.t & VT_BTYPE) == VT_FUNC || is_reference_type(&actual));
  if (is_reference_type(&actual)) actual = *pointed_type(&actual);
  actual.t &= ~(VT_STORAGE | VT_RVALUE_REFERENCE);
  if (is_reference_type(&pattern))
  {
    CType *pointee = pointed_type(&pattern);
    int forwarding = (pattern.t & VT_RVALUE_REFERENCE)
        && (pointee->t & VT_BTYPE) == VT_INT
        && !(pointee->t & (VT_CONSTANT | VT_VOLATILE));
    int rvalue = (pattern.t & VT_RVALUE_REFERENCE) && !(forwarding && lvalue);
    if (rvalue && lvalue)
      cprime_error("lvalue return expression cannot bind to auto rvalue reference");
    if (!rvalue && !lvalue && !(pointee->t & VT_CONSTANT))
      cprime_error("rvalue return expression cannot bind to auto lvalue reference");
    result = deduce_placeholder_return_pattern(pointee, &actual);
    mk_reference(&result);
    if (rvalue) result.t |= VT_RVALUE_REFERENCE;
  }
  else
  {
    actual.t &= ~(VT_CONSTANT | VT_VOLATILE);
    if (actual.t & VT_ARRAY) actual.t &= ~VT_ARRAY;
    if ((actual.t & VT_BTYPE) == VT_FUNC) mk_pointer(&actual);
    result = deduce_placeholder_return_pattern(&pattern, &actual);
  }
  if ((result.t & VT_BTYPE) == VT_PTR)
    result.ref = template_stable_pointer_ref(&result);
  return result;
}

static void deduce_decltype_auto_return(CType *result)
{
  TokenString *expression = tok_str_alloc(), *parent;
  const int *parent_ptr;
  int parens = 0, brackets = 0, braces = 0, token;
  CValue value;
  tok_str_add(expression, '(');
  while (tok != TOK_EOF && (tok != ';' || parens || brackets || braces)) {
    if (tok == '(') ++parens; else if (tok == ')') --parens;
    else if (tok == '[') ++brackets; else if (tok == ']') --brackets;
    else if (tok == '{') ++braces; else if (tok == '}') --braces;
    tok_str_add_tok(expression);
    next();
  }
  tok_str_add(expression, ')');
  tok_str_add(expression, TOK_EOF);
  parent = macro_stack; parent_ptr = macro_ptr; token = tok; value = tokc;
  begin_macro(expression, 1);
  next();
  parse_decltype_type(result);
  if ((result->t & VT_BTYPE) == VT_PTR || (result->t & VT_BTYPE) == VT_FUNC)
    result->ref = template_stable_pointer_ref(result);
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr; tok = token; tokc = value;
}

static void lambda_note_return(void)
{
  CType result;
  result.t = VT_VOID;
  result.ref = NULL;
  if (tok != ';')
  {
    if (tok == '{')
    {
      skip_or_save_block(NULL);
      result = lambda_probe->return_type;
      if (is_reference_type(&result)) result = *pointed_type(&result);
      result.t &= ~(VT_CONSTANT | VT_VOLATILE | VT_STORAGE
                    | VT_RVALUE_REFERENCE);
    }
    else if (lambda_probe->return_deduction_kind == 2)
      deduce_decltype_auto_return(&result);
    else
    {
      gexpr();
      if (lambda_probe->return_deduction_kind)
        result = deduce_placeholder_return_expression(lambda_probe);
      else
      {
        result = vtop->type;
        if (is_reference_type(&result)) result = *pointed_type(&result);
        result.t &= ~(VT_CONSTANT | VT_VOLATILE | VT_STORAGE | VT_RVALUE_REFERENCE);
        if (result.t & VT_ARRAY) result.t &= ~VT_ARRAY;
        if ((result.t & VT_BTYPE) == VT_PTR)
          result.ref = template_stable_pointer_ref(&result);
      }
      vpop();
    }
  }
  else if (lambda_probe->return_deduction_kind == 1
           && (lambda_probe->return_pattern.t & VT_BTYPE) == VT_PTR)
    cprime_error("empty return does not match auto pointer or reference declarator");
  if (!lambda_probe->explicit_return)
  {
    if (lambda_probe->return_seen
        && !is_compatible_types(&lambda_probe->return_type, &result))
      cprime_error("inconsistent deduced lambda return types");
    lambda_probe->return_type = result;
    lambda_probe->return_seen = 1;
  }
  skip(';');
}

static int lambda_lexical_owner(void)
{
  LambdaExpression *enclosing;
  const char *function_name = funcname ? funcname : "";
  if (lambda_probe)
    return lambda_probe->owner;
  /* A lambda body is probed in its enclosing function, then emitted as the
     closure's call operator. Nested expressions retain their lexical owner
     across both parses, including separate function-template instances. */
  for (enclosing = lambda_expressions; enclosing; enclosing = enclosing->next)
  {
    const char *class_name = get_tok_str(enclosing->class_tok, NULL);
    size_t length = strlen(class_name);
    if (!strncmp(function_name, class_name, length)
        && function_name[length] == '_')
      return enclosing->owner;
  }
  return tok_alloc_const(function_name);
}

static void append_captureless_lambda_conversion(TokenString *declaration,
                                                  LambdaExpression *lambda,
                                                  TokenString *parameters)
{
  CType function = make_func_type_from_saved_params(&lambda->return_type, parameters);
  CType pointer = function;
  Sym *parameter;
  int index, pointer_tok, invoke_tok = tok_alloc_const("__cpc_lambda_invoke");
  int object_tok = tok_alloc_const("__cpc_lambda_object");
  if (function.ref->f.func_type == FUNC_ELLIPSIS) return;
  mk_pointer(&pointer);
  pointer_tok = template_exact_ctype_typedef_tok(&pointer);
  tok_str_add(declaration, TOK_STATIC);
  add_ctype_tokens(declaration, &lambda->return_type);
  tok_str_add(declaration, invoke_tok);
  tok_str_add(declaration, '(');
  for (parameter = function.ref->next, index = 0; parameter;
       parameter = parameter->next, ++index)
  {
    char name[64];
    snprintf(name, sizeof(name), "__cpc_lambda_arg_%d", index);
    if (index) tok_str_add(declaration, ',');
    add_ctype_declarator_tokens(declaration, &parameter->type, tok_alloc_const(name));
  }
  tok_str_add(declaration, ')');
  tok_str_add(declaration, '{');
  tok_str_add(declaration, lambda->class_tok);
  tok_str_add(declaration, object_tok);
  tok_str_add(declaration, ';');
  tok_str_add(declaration, TOK_RETURN);
  tok_str_add(declaration, object_tok);
  tok_str_add(declaration, '(');
  for (parameter = function.ref->next, index = 0; parameter;
       parameter = parameter->next, ++index)
  {
    CType forwarded = parameter->type;
    char name[64];
    snprintf(name, sizeof(name), "__cpc_lambda_arg_%d", index);
    if (index) tok_str_add(declaration, ',');
    if (!is_reference_type(&forwarded))
    {
      mk_reference(&forwarded);
      forwarded.t |= VT_RVALUE_REFERENCE;
    }
    tok_str_add(declaration, tok_alloc_const("static_cast"));
    tok_str_add(declaration, TOK_LT);
    add_ctype_tokens(declaration, &forwarded);
    tok_str_add(declaration, TOK_GT);
    tok_str_add(declaration, '(');
    tok_str_add(declaration, tok_alloc_const(name));
    tok_str_add(declaration, ')');
  }
  tok_str_add(declaration, ')');
  tok_str_add(declaration, ';');
  tok_str_add(declaration, '}');
  tok_str_add(declaration, TOK_OPERATOR);
  tok_str_add(declaration, pointer_tok);
  tok_str_add(declaration, '(');
  tok_str_add(declaration, ')');
  tok_str_add(declaration, TOK_CONST1);
  tok_str_add(declaration, '{');
  tok_str_add(declaration, TOK_RETURN);
  tok_str_add(declaration, '&');
  tok_str_add(declaration, lambda->class_tok);
  tok_str_add(declaration, ':');
  tok_str_add(declaration, ':');
  tok_str_add(declaration, invoke_tok);
  tok_str_add(declaration, ';');
  tok_str_add(declaration, '}');
}

static void parse_cpp_lambda_expression(void)
{
  int marker = tok == '[' ? new_lambda_marker() : tok;
  int owner = lambda_lexical_owner();
  int enclosing_class = active_member_class_tok;
  int i, depth, parameter_default = 0, saved_tok;
  CValue saved_tokc;
  LambdaExpression current, *lambda;
  TokenString *parameters = tok_str_alloc(), *body = NULL, *declaration, *value;
  TokenString *parent;
  const int *parent_ptr;
  memset(&current, 0, sizeof(current));
  current.marker = marker;
  current.owner = owner;
  current.outer_scope = local_scope;
  current.outer_this = find_cpp_this_symbol();
  current.enclosing_class_tok = enclosing_class;
  current.return_type.t = VT_VOID;
  for (lambda = lambda_expressions; lambda; lambda = lambda->next)
    if (lambda->marker == marker && lambda->owner == owner) break;
  next();
  if (tok == '=' || tok == '&')
  {
    int mode = tok;
    next();
    if (tok == ',' || tok == ']')
    {
      current.default_capture = mode;
      if (tok == ',') next();
    }
    else
    {
      if (mode != '&') cprime_error("invalid lambda capture default");
      unget_tok(tok);
      tok = '&';
    }
  }
  while (tok != ']')
  {
    int by_reference = 0;
    int name_tok;
    Sym *source;
    if (tok == '&') { by_reference = 1; next(); }
    if (tok < TOK_UIDENT) cprime_error("lambda capture name expected");
    name_tok = tok;
    next();
    if (tok == '=')
    {
      TokenString *init_tokens = tok_str_alloc();
      next();
      skip_or_save_block(&init_tokens);
      /* Init captures are parsed for grammar; their initializer does not
         introduce a closure field in this lowering. */
      tok_str_free(init_tokens);
    }
    else if (!lambda)
    {
      for (i = 0; i < current.count; ++i)
        if (current.captures[i].name == name_tok)
          cprime_error("duplicate lambda capture '%s'",
                       get_tok_str(name_tok, NULL));
      source = name_tok == tok_alloc_const("this")
                 ? current.outer_this : sym_find(name_tok);
      if (name_tok == tok_alloc_const("this") && by_reference)
        cprime_error("this must be captured by value");
      if (!source || !source->sym_scope)
        cprime_error("lambda capture '%s' must name an automatic variable (function '%s', scope %d)",
                     get_tok_str(name_tok, NULL), funcname, local_scope);
      add_lambda_capture(&current, source, by_reference);
    }
    if (tok == ']') break;
    skip(',');
  }
  next();
  if (tok == '(')
  {
    next();
    depth = 1;
    while (depth && tok != TOK_EOF)
    {
      if (tok == TOK_AUTO && !parameter_default)
      {
        int parameter_index = current.generic_param_count;
        tok_str_add(parameters, new_lambda_generic_param(&current));
        next();
        if (tok == TOK_DOTS)
          current.generic_param_is_pack[parameter_index] = 1;
        continue;
      }
      if (tok == '(') ++depth;
      if (tok == ')' && !--depth) { next(); break; }
      if (tok == '=' && depth == 1) parameter_default = 1;
      else if (tok == ',' && depth == 1) parameter_default = 0;
      tok_str_add_tok(parameters);
      next();
    }
  }
  tok_str_add(parameters, TOK_EOF);
  if (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "mutable"))
  { current.mutable_call = 1; next(); }
  if (tok >= TOK_UIDENT && !strcmp(get_tok_str(tok, NULL), "noexcept")) next();
  if (tok == TOK_ARROW)
  {
    AttributeDef attr;
    int name = 0;
    next();
    current.return_deduction_kind = tok == TOK_AUTO ? 1 : 0;
    memset(&attr, 0, sizeof(attr));
    if (!parse_btype(&current.return_type, &attr, 0)) expect("lambda return type");
    type_decl(&current.return_type, &attr, &name, TYPE_ABSTRACT);
    current.return_pattern = current.return_type;
    current.explicit_return = !current.return_deduction_kind;
  }
  if (tok != '{') expect("lambda body");
  skip_or_save_block(&body);
  {
    TokenString *folded =
      fold_lambda_unevaluated_outer_sizes(&current, body);
    if (folded != body)
    {
      tok_str_free(body);
      body = folded;
    }
  }
  saved_tok = tok;
  saved_tokc = tokc;
  if (!lambda)
  {
    char class_name[96];
    snprintf(class_name, sizeof(class_name), "__cpc_closure_%u", anon_sym++);
    current.class_tok = current.body_owner_tok = tok_alloc_const(class_name);
    probe_lambda_body(&current, parameters, body);
    lambda = cprime_malloc(sizeof(*lambda));
    *lambda = current;
    lambda->next = lambda_expressions;
    lambda_expressions = lambda;
    declaration = tok_str_alloc();
    tok_str_add(declaration, TOK_STRUCT);
    tok_str_add(declaration, lambda->class_tok);
    tok_str_add(declaration, '{');
    for (i = 0; i < lambda->count; ++i)
    {
      if (!add_ctype_declarator_tokens(declaration, &lambda->captures[i].type,
                                       lambda_capture_field_tok(lambda, i)))
        cprime_error("unsupported lambda capture type");
      tok_str_add(declaration, ';');
    }
    tok_str_add(declaration, lambda->class_tok);
    tok_str_add(declaration, '(');
    for (i = 0; i < lambda->count; ++i)
    {
      CType param_type = lambda->captures[i].type;
      int arg = make_joined_name_tok(tok_alloc_const("__cpc_capture_arg"),
                                     lambda->captures[i].name, "_", "");
      if (i) tok_str_add(declaration, ',');
      if (!lambda->captures[i].by_reference)
      {
        param_type.t |= VT_CONSTANT;
        mk_pointer(&param_type);
        param_type.t |= VT_REFERENCE;
      }
      if (!add_ctype_declarator_tokens(declaration, &param_type, arg))
        cprime_error("unsupported lambda capture parameter type");
    }
    tok_str_add(declaration, ')');
    for (i = 0; i < lambda->count; ++i)
    {
      int arg = make_joined_name_tok(tok_alloc_const("__cpc_capture_arg"),
                                     lambda->captures[i].name, "_", "");
      tok_str_add(declaration, i ? ',' : ':');
      tok_str_add(declaration, lambda_capture_field_tok(lambda, i));
      tok_str_add(declaration, '(');
      tok_str_add(declaration, arg);
      tok_str_add(declaration, ')');
    }
    tok_str_add(declaration, '{');
    tok_str_add(declaration, '}');
    if (lambda->generic)
    {
      tok_str_add(declaration, tok_alloc_const("template"));
      tok_str_add(declaration, TOK_LT);
      for (i = 0; i < lambda->generic_param_count; ++i)
      {
        if (i) tok_str_add(declaration, ',');
        tok_str_add(declaration, TOK_CLASS);
        if (lambda->generic_param_is_pack[i])
          tok_str_add(declaration, TOK_DOTS);
        tok_str_add(declaration, lambda->generic_param_toks[i]);
      }
      tok_str_add(declaration, TOK_GT);
    }
    if (lambda->generic && !lambda->explicit_return)
      tok_str_add(declaration, TOK_AUTO);
    else
    {
      int return_alias = (lambda->return_type.t & VT_BTYPE) == VT_STRUCT
        ? template_exact_ctype_typedef_tok(&lambda->return_type) : 0;
      if (return_alias)
        tok_str_add(declaration, return_alias);
      else
        add_ctype_tokens(declaration, &lambda->return_type);
    }
    tok_str_add(declaration, TOK_OPERATOR);
    tok_str_add(declaration, '(');
    tok_str_add(declaration, ')');
    tok_str_add(declaration, '(');
    tok_str_append_without_eof(declaration, parameters);
    tok_str_add(declaration, ')');
    if (!lambda->mutable_call) tok_str_add(declaration, TOK_CONST1);
    for (i = 0; i < lambda->count; ++i)
      if (lambda->captures[i].source == lambda->outer_this) break;
    if (i < lambda->count)
    {
      TokenString remainder = *body;
      int first = tok_str_next_non_linenum_index(body->str, body->len, 0);
      if (body->str[first] != '{') cprime_error("lambda compound body expected");
      tok_str_add(declaration, '{');
      if (!lambda->generic)
      {
        if (!add_ctype_declarator_tokens(declaration, &lambda->outer_this->type,
                                         tok_alloc_const("__cpc_lambda_this")))
          cprime_error("unsupported enclosing object type in lambda");
        tok_str_add(declaration, '=');
        tok_str_add(declaration, lambda_capture_field_tok(lambda, i));
        tok_str_add(declaration, ';');
      }
      remainder.str += first + 1;
      remainder.len -= first + 1;
      {
        int body_i, body_scope_depth = 1;
        for (body_i = 0; body_i < remainder.len && remainder.str[body_i] != TOK_EOF; ++body_i) {
          int body_tok = remainder.str[body_i];
          if (TOK_HAS_VALUE(body_tok))
            tok_str_add_record(declaration, remainder.str, remainder.len, &body_i);
          else if (body_tok == '{')
          {
            tok_str_add(declaration, body_tok);
            ++body_scope_depth;
          }
          else if (body_tok == '}')
          {
            if (body_scope_depth) --body_scope_depth;
            tok_str_add(declaration, body_tok);
          }
          else if (body_tok == tok_alloc_const("this")
                   || body_tok == (lambda->outer_this->v & ~SYM_FIELD))
            tok_str_add(declaration,
                lambda->generic ? lambda_capture_field_tok(lambda, i)
                                : tok_alloc_const("__cpc_lambda_this"));
          else if (lambda->generic && body_tok >= TOK_UIDENT)
          {
            int prev = tok_str_prev_token_index(remainder.str, remainder.len,
                                                body_i);
            int next = tok_str_next_non_linenum_index(remainder.str,
                                                       remainder.len,
                                                       body_i + 1);
            int qualified = prev >= 0
                && (remainder.str[prev] == '.'
                    || remainder.str[prev] == TOK_ARROW
                    || remainder.str[prev] == ':');
            int nested_name = next < remainder.len
                && remainder.str[next] == ':' && !qualified;
            if (!qualified && !nested_name
                && !lambda_body_name_is_local(lambda, body_tok,
                                              body_scope_depth,
                                              first + 1 + body_i)
                && !lambda_symbol_is_outer_automatic(lambda,
                                                     sym_find(body_tok))
                && lambda_is_enclosing_member(enclosing_class, body_tok))
            {
              tok_str_add(declaration, lambda_capture_field_tok(lambda, i));
              tok_str_add(declaration, TOK_ARROW);
            }
            tok_str_add(declaration, body_tok);
          }
          else
            tok_str_add(declaration, body_tok);
        }
      }
    }
    else tok_str_append_without_eof(declaration, body);
    if (!lambda->generic && !lambda->count && !lambda->default_capture)
      append_captureless_lambda_conversion(declaration, lambda, parameters);
    tok_str_add(declaration, '}');
    tok_str_add(declaration, ';');
    tok_str_add(declaration, TOK_EOF);
    publish_template_member_proto(declaration, -1);
    lambda->class_tok = find_current_namespace_tok(lambda->class_tok);
    {
      CppRecordDeclInfo *record =
        (CppRecordDeclInfo *)lookup_cpp_record_decl(lambda->class_tok);
      /* Deferred closure bodies retain the lexical class scope of the
         lambda expression, including captureless lambdas in static members. */
      if (record) record->lexical_owner_tok = enclosing_class;
    }
  }
  for (i = 0; i < lambda->static_count; ++i) {
    Sym *source = sym_find(lambda->static_names[i]);
    if (source && source->sym_scope && (source->type.t & VT_STATIC))
      lambda_note_static(lambda, source);
  }
  tok_str_free(parameters);
  tok_str_free(body);
  value = tok_str_alloc();
  tok_str_add(value, '(');
  for (i = 0; i < lambda->count; ++i)
  {
    if (i) tok_str_add(value, ',');
    tok_str_add(value, lambda->captures[i].source == lambda->outer_this
        && current.outer_this ? current.outer_this->v & ~SYM_FIELD
                              : lambda->captures[i].name);
  }
  tok_str_add(value, ')');
  tok_str_add(value, TOK_EOF);
  parent = macro_stack;
  parent_ptr = macro_ptr;
  begin_macro(value, 1);
  next();
  if (!try_parse_cpp_functional_constructor(lambda->class_tok))
    cprime_error("could not construct lambda closure");
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr;
  tok = saved_tok;
  tokc = saved_tokc;
}
