/* Lambda expressions have distinct closure types, including when an
   expression is first replayed for deduction and later for code emission. */
typedef struct LambdaCapture {
  int name, by_reference;
  CType type;
  Sym *source;
} LambdaCapture;

typedef struct LambdaExpression {
  int marker, owner, class_tok, count, default_capture, mutable_call;
  int return_seen, explicit_return, outer_scope;
  CType return_type;
  LambdaCapture captures[64];
  struct LambdaExpression *next, *parent_probe;
} LambdaExpression;

static LambdaExpression *lambda_expressions, *lambda_probe;
static unsigned lambda_expression_counter;
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
        if (capture < TOK_UIDENT && capture != '&' && capture != '='
            && capture != ',') break;
        if (TOK_HAS_VALUE(capture)) break;
      }
      if (j < expression->len && expression->str[j] == ']')
        expression->str[i] = new_lambda_marker();
    }
    previous = t;
  }
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

static Sym *lambda_capture_symbol(Sym *symbol)
{
  LambdaCapture *capture;
  CType visible_type;
  if (!lambda_probe || !symbol || !symbol->sym_scope
      || symbol->sym_scope > lambda_probe->outer_scope
      || (symbol->type.t & (VT_STATIC | VT_EXTERN | VT_TYPEDEF))
      || (symbol->type.t & VT_BTYPE) == VT_FUNC || IS_ENUM_VAL(symbol->type.t))
    return symbol;
  if (!lambda_probe->default_capture)
    cprime_error("'%s' is not captured by the lambda",
                 get_tok_str(symbol->v & ~SYM_FIELD, NULL));
  capture = add_lambda_capture(lambda_probe, symbol,
                               lambda_probe->default_capture == '&');
  visible_type = capture->type;
  if (!capture->by_reference && !lambda_probe->mutable_call)
    visible_type.t |= VT_CONSTANT;
  return sym_push(capture->name, &visible_type, VT_LOCAL | VT_LVAL, 0);
}

static void lambda_note_return(void)
{
  CType result;
  result.t = VT_VOID;
  result.ref = NULL;
  if (tok != ';')
  {
    gexpr();
    result = vtop->type;
    if (is_reference_type(&result)) result = *pointed_type(&result);
    result.t &= ~(VT_CONSTANT | VT_VOLATILE | VT_STORAGE | VT_RVALUE_REFERENCE);
    if (result.t & VT_ARRAY) result.t &= ~VT_ARRAY;
    if ((result.t & VT_BTYPE) == VT_PTR)
      result.ref = template_stable_pointer_ref(&result);
    vpop();
  }
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

static void parse_cpp_lambda_expression(void)
{
  int marker = tok == '[' ? new_lambda_marker() : tok;
  int owner = tok_alloc_const(funcname ? funcname : "");
  int i, depth, saved_tok;
  CValue saved_tokc;
  LambdaExpression current, *lambda;
  TokenString *parameters = tok_str_alloc(), *body = NULL, *declaration, *value;
  TokenString *parent;
  const int *parent_ptr;
  memset(&current, 0, sizeof(current));
  current.marker = marker;
  current.owner = owner;
  current.outer_scope = local_scope;
  current.return_type.t = VT_VOID;
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
    Sym *source;
    if (tok == '&') { by_reference = 1; next(); }
    if (tok < TOK_UIDENT) cprime_error("lambda capture name expected");
    source = sym_find(tok);
    if (!source || !source->sym_scope)
      cprime_error("lambda capture '%s' must name an automatic variable (function '%s', scope %d)",
                   get_tok_str(tok, NULL), funcname, local_scope);
    add_lambda_capture(&current, source, by_reference);
    next();
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
      if (tok == '(') ++depth;
      if (tok == ')' && !--depth) { next(); break; }
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
    memset(&attr, 0, sizeof(attr));
    if (!parse_btype(&current.return_type, &attr, 0)) expect("lambda return type");
    type_decl(&current.return_type, &attr, &name, TYPE_ABSTRACT);
    current.explicit_return = 1;
  }
  if (tok != '{') expect("lambda body");
  skip_or_save_block(&body);
  saved_tok = tok;
  saved_tokc = tokc;
  for (lambda = lambda_expressions; lambda; lambda = lambda->next)
    if (lambda->marker == marker && lambda->owner == owner) break;
  if (!lambda)
  {
    char class_name[96];
    probe_lambda_body(&current, parameters, body);
    lambda = cprime_malloc(sizeof(*lambda));
    *lambda = current;
    snprintf(class_name, sizeof(class_name), "__cpc_closure_%u", anon_sym++);
    lambda->class_tok = tok_alloc_const(class_name);
    lambda->next = lambda_expressions;
    lambda_expressions = lambda;
    declaration = tok_str_alloc();
    tok_str_add(declaration, TOK_STRUCT);
    tok_str_add(declaration, lambda->class_tok);
    tok_str_add(declaration, '{');
    for (i = 0; i < lambda->count; ++i)
    {
      if (!add_ctype_declarator_tokens(declaration, &lambda->captures[i].type,
                                       lambda->captures[i].name))
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
      add_ctype_declarator_tokens(declaration, &param_type, arg);
    }
    tok_str_add(declaration, ')');
    for (i = 0; i < lambda->count; ++i)
    {
      int arg = make_joined_name_tok(tok_alloc_const("__cpc_capture_arg"),
                                     lambda->captures[i].name, "_", "");
      tok_str_add(declaration, i ? ',' : ':');
      tok_str_add(declaration, lambda->captures[i].name);
      tok_str_add(declaration, '(');
      tok_str_add(declaration, arg);
      tok_str_add(declaration, ')');
    }
    tok_str_add(declaration, '{');
    tok_str_add(declaration, '}');
    add_ctype_tokens(declaration, &lambda->return_type);
    tok_str_add(declaration, TOK_OPERATOR);
    tok_str_add(declaration, '(');
    tok_str_add(declaration, ')');
    tok_str_add(declaration, '(');
    tok_str_append_without_eof(declaration, parameters);
    tok_str_add(declaration, ')');
    if (!lambda->mutable_call) tok_str_add(declaration, TOK_CONST1);
    tok_str_append_without_eof(declaration, body);
    tok_str_add(declaration, '}');
    tok_str_add(declaration, ';');
    tok_str_add(declaration, TOK_EOF);
    publish_template_member_proto(declaration);
    lambda->class_tok = find_current_namespace_tok(lambda->class_tok);
  }
  tok_str_free(parameters);
  tok_str_free(body);
  value = tok_str_alloc();
  tok_str_add(value, '(');
  for (i = 0; i < lambda->count; ++i)
  {
    if (i) tok_str_add(value, ',');
    tok_str_add(value, lambda->captures[i].name);
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
