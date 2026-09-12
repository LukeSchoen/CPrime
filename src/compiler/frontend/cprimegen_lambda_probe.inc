/* A generic lambda's body is dependent on its invented template parameters,
   so it cannot be type-checked at the definition site merely to discover
   implicit captures.  This pass resolves the body's block-scope names only:
   identifier uses that still name an enclosing automatic object are fed to
   the ordinary lambda capture bookkeeping, while local declarations shadow
   them without requiring the declaration's type to be complete. */
typedef struct LambdaScanState {
  LambdaExpression *lambda;
  TokenString *body;
  int scope_depth, outer_this_seen;
} LambdaScanState;

static int lambda_scan_is_type_start(int token, LambdaExpression *lambda)
{
  int i;

  switch (token)
  {
  case TOK_VOID:
  case TOK_CHAR:
  case TOK_SHORT:
  case TOK_INT:
  case TOK_LONG:
  case TOK_BOOL:
  case TOK_BOOL2:
  case TOK_FLOAT:
  case TOK_DOUBLE:
  case TOK_ENUM:
  case TOK_STRUCT:
  case TOK_CLASS:
  case TOK_UNION:
  case TOK__Atomic:
  case TOK_CONST1:
  case TOK_CONST2:
  case TOK_CONST3:
  case TOK_VOLATILE1:
  case TOK_VOLATILE2:
  case TOK_VOLATILE3:
  case TOK_SIGNED1:
  case TOK_SIGNED2:
  case TOK_SIGNED3:
  case TOK_UNSIGNED:
  case TOK_AUTO:
  case TOK_DECLTYPE:
  case TOK_DECLTYPE2:
  case TOK_UNDERLYING_TYPE:
    return 1;
  default:
    break;
  }
  if (token < TOK_UIDENT)
    return 0;
  for (i = 0; i < lambda->generic_param_count; ++i)
    if (token == lambda->generic_param_toks[i])
      return 1;
  if (is_namespace_tok(token) || find_class_template_def(token))
    return 1;
  if (struct_find(token))
    return 1;
  {
    Sym *symbol = sym_find(token);
    if (symbol && (symbol->type.t & VT_TYPEDEF))
      return 1;
  }
  return token == tok_alloc_const("typename");
}

static int lambda_scan_next_index(TokenString *body, int index)
{
  return tok_str_next_non_linenum_index(body->str, body->len, index);
}

static int lambda_scan_previous_index(TokenString *body, int index)
{
  return tok_str_prev_token_index(body->str, body->len, index);
}

static int lambda_scan_name_is_local(LambdaScanState *state, int name,
                                     int index)
{
  return lambda_body_name_is_local(state->lambda, name, state->scope_depth,
                                   index);
}

static int lambda_scan_declaration_end(TokenString *body, int start)
{
  int i, paren = 0, bracket = 0, brace = 0;

  for (i = start; i < body->len; ++i)
  {
    int token = body->str[i];
    if (TOK_HAS_VALUE(token))
    {
      i += tok_str_value_extra_words(body->str, body->len, i);
      continue;
    }
    if (token == '(')
      ++paren;
    else if (token == ')')
    {
      if (!paren)
        return i;
      --paren;
    }
    else if (token == '[')
      ++bracket;
    else if (token == ']')
    {
      if (bracket) --bracket;
    }
    else if (token == '{')
      ++brace;
    else if (token == '}')
    {
      if (brace) --brace;
      else return i;
    }
    else if ((token == ';' || token == ')')
             && !paren && !bracket && !brace)
      return i;
  }
  return body->len;
}

static int lambda_scan_is_declarator_name(TokenString *body, int index,
                                          int first_type)
{
  int token, previous, next;

  if (index < 0 || index >= body->len)
    return 0;
  token = body->str[index];
  if (token < TOK_UIDENT)
    return 0;
  previous = lambda_scan_previous_index(body, index);
  next = lambda_scan_next_index(body, index + 1);
  if (previous >= 0
      && (body->str[previous] == '.' || body->str[previous] == TOK_ARROW
          || body->str[previous] == ':'))
    return 0;
  if (next >= body->len)
    return 0;
  switch (body->str[next])
  {
  case '=':
  case ',':
  case ';':
  case '(':
  case '[':
  case '{':
  case ')':
  case ':':
    break;
  default:
    return 0;
  }
  /* The first name in a declaration is the type.  It can also be the
     declarator when the same typedef name is used as a variable name, which
     is valid but vanishingly rare in a lambda body. */
  if (index == first_type)
    return 0;
  return 1;
}

static void lambda_scan_collect_declarators(LambdaScanState *state, int start,
                                            int end, int scope_depth)
{
  TokenString *body = state->body;
  int i = start, first_type = -1;

  while (i < end)
  {
    int token = body->str[i];
    if (token == TOK_LINENUM)
    {
      ++i;
      continue;
    }
    if (TOK_HAS_VALUE(token))
    {
      i += 1 + tok_str_value_extra_words(body->str, body->len, i);
      continue;
    }
    if (lambda_scan_is_type_start(token, state->lambda))
    {
      first_type = i;
      break;
    }
    ++i;
  }
  if (first_type < 0)
    return;

  i = first_type;
  while (i < end)
  {
    int found = -1, j, k;
    int paren = 0, bracket = 0, brace = 0;
    for (j = i; j < end; ++j)
    {
      int token = body->str[j];
      if (TOK_HAS_VALUE(token))
      {
        j += tok_str_value_extra_words(body->str, body->len, j);
        continue;
      }
      if (token == '(')
        ++paren;
      else if (token == ')')
      {
        if (paren) --paren;
        else break;
      }
      else if (token == '[')
        ++bracket;
      else if (token == ']')
      {
        if (bracket) --bracket;
      }
      else if (token == '{')
        ++brace;
      else if (token == '}')
      {
        if (brace) --brace;
        else break;
      }
      else if (!paren && !bracket && !brace
               && (token == ',' || token == '=' || token == ';'
                   || token == ':'))
        break;
      if (lambda_scan_is_declarator_name(body, j, first_type))
      {
        found = j;
        lambda_note_body_declaration(state->lambda, body->str[found],
                                     scope_depth, found);
        break;
      }
    }

    k = found >= 0 ? found + 1 : j;
    paren = bracket = brace = 0;
    for (; k < end; ++k)
    {
      int token = body->str[k];
      if (TOK_HAS_VALUE(token))
      {
        k += tok_str_value_extra_words(body->str, body->len, k);
        continue;
      }
      if (token == '(')
        ++paren;
      else if (token == ')')
      {
        if (paren) --paren;
        else break;
      }
      else if (token == '[')
        ++bracket;
      else if (token == ']')
      {
        if (bracket) --bracket;
      }
      else if (token == '{')
        ++brace;
      else if (token == '}')
      {
        if (brace) --brace;
        else break;
      }
      else if (!paren && !bracket && !brace && token == ',')
        break;
    }
    if (k >= end)
      break;
    i = k + 1;
  }
}

static void lambda_scan_identifier(LambdaScanState *state, int index)
{
  TokenString *body = state->body;
  int token = body->str[index];
  int previous = lambda_scan_previous_index(body, index);
  int next = lambda_scan_next_index(body, index + 1);
  Sym *symbol;

  if (token < TOK_UIDENT || lambda_scan_name_is_local(state, token, index))
    return;
  if (previous >= 0
      && (body->str[previous] == '.' || body->str[previous] == TOK_ARROW
          || body->str[previous] == ':'))
    return;
  if (next < body->len && body->str[next] == ':'
      && (previous < 0 || body->str[previous] != '?'))
    return;
  if (token == tok_alloc_const("this"))
  {
    if (state->lambda->outer_this && state->lambda->default_capture
        && !state->outer_this_seen)
    {
      lambda_capture_symbol(state->lambda->outer_this);
      state->outer_this_seen = 1;
    }
    return;
  }
  symbol = sym_find(token);
  if (symbol && symbol->sym_scope > state->lambda->outer_scope)
    return;
  if (lambda_symbol_is_outer_automatic(state->lambda, symbol))
  {
    lambda_capture_symbol(symbol);
    return;
  }
  if (state->lambda->outer_this && state->lambda->default_capture
      && !state->outer_this_seen
      && lambda_is_enclosing_member(state->lambda->enclosing_class_tok,
                                    token))
  {
    lambda_capture_symbol(state->lambda->outer_this);
    state->outer_this_seen = 1;
    return;
  }
  if (symbol)
    lambda_capture_symbol(symbol);
}

static void probe_generic_lambda_captures(LambdaExpression *lambda,
                                           TokenString *body)
{
  LambdaScanState state;
  int i = 0, statements = 1;

  memset(&state, 0, sizeof(state));
  state.lambda = lambda;
  state.body = body;
  for (i = 0; i < lambda->count; ++i)
    if (lambda->captures[i].source == lambda->outer_this)
      state.outer_this_seen = 1;
  /* First collect block-scope declarations in source order.  Visibility is
     checked against the declaration's scope depth and token index, so a use
     before a later shadowing declaration still sees the enclosing object. */
  i = 0;
  statements = 1;
  state.scope_depth = 0;
  while (i < body->len)
  {
    int token = body->str[i];
    int next;
    if (TOK_HAS_VALUE(token))
    {
      i += 1 + tok_str_value_extra_words(body->str, body->len, i);
      continue;
    }
    if (token == '{')
    {
      ++state.scope_depth;
      statements = 1;
      ++i;
      continue;
    }
    if (token == '}')
    {
      if (state.scope_depth) --state.scope_depth;
      statements = 1;
      ++i;
      continue;
    }
    if (token == ';')
    {
      statements = 1;
      ++i;
      continue;
    }
    if (statements && lambda_scan_is_type_start(token, lambda))
    {
      int end = lambda_scan_declaration_end(body, i);
      lambda_scan_collect_declarators(&state, i, end, state.scope_depth);
      statements = 0;
      ++i;
      continue;
    }
    next = lambda_scan_next_index(body, i);
    if (token == '(' && i > 0
        && (body->str[i - 1] == TOK_FOR || body->str[i - 1] == TOK_IF
            || body->str[i - 1] == TOK_WHILE
            || body->str[i - 1] == TOK_SWITCH
            || body->str[i - 1] == TOK_CATCH))
      statements = 1;
    else
      statements = 0;
    if (next > i)
      i = next;
    else
      ++i;
  }

  /* Second pass: any unqualified identifier that still resolves to an
     enclosing automatic object is an odr-use for capture purposes. */
  i = 0;
  state.scope_depth = 0;
  while (i < body->len)
  {
    int token = body->str[i];
    if (TOK_HAS_VALUE(token))
    {
      i += 1 + tok_str_value_extra_words(body->str, body->len, i);
      continue;
    }
    if (token == '{')
      ++state.scope_depth;
    else if (token == '}')
    {
      if (state.scope_depth) --state.scope_depth;
    }
    else if ((token == TOK_SIZEOF || token == TOK_ALIGNOF1
              || token == TOK_ALIGNOF2 || token == TOK_ALIGNOF3
              || token == TOK_ALIGNOF4 || token == TOK_DECLTYPE
              || token == TOK_DECLTYPE2)
             && i + 1 < body->len && body->str[i + 1] == '(')
    {
      int depth = 0;
      int j = i + 1;
      do
      {
        if (body->str[j] == '(')
          ++depth;
        else if (body->str[j] == ')')
          --depth;
        ++j;
      } while (j < body->len && depth > 0);
      /* Keep the deferred member body able to resolve a name that appears
         only in an unevaluated operand. */
      while (++i < j - 1)
      {
        Sym *symbol;
        if (TOK_HAS_VALUE(body->str[i]))
          i += tok_str_value_extra_words(body->str, body->len, i);
        else if (body->str[i] >= TOK_UIDENT
                 && (symbol = sym_find(body->str[i])) != NULL
                 && lambda_symbol_is_outer_automatic(lambda, symbol))
          add_lambda_capture(lambda, symbol, 0);
      }
      i = j;
      continue;
    }
    else if (token >= TOK_UIDENT)
      lambda_scan_identifier(&state, i);
    ++i;
  }
}

static TokenString *lambda_parameters_without_generic_packs(
    LambdaExpression *lambda, TokenString *parameters)
{
  TokenString *copy;
  int i, needed = 0;

  if (!lambda->generic)
    return parameters;
  for (i = 0; i < parameters->len; ++i)
    if (parameters->str[i] == TOK_DOTS)
    {
      int previous = lambda_scan_previous_index(parameters, i);
      int parameter;
      for (parameter = 0; parameter < lambda->generic_param_count; ++parameter)
        if (previous >= 0
            && parameters->str[previous] == lambda->generic_param_toks[parameter]
            && lambda->generic_param_is_pack[parameter])
          needed = 1;
    }
  if (!needed)
    return parameters;

  copy = tok_str_alloc();
  for (i = 0; i < parameters->len; ++i)
  {
    int token = parameters->str[i];
    int parameter;
    if (TOK_HAS_VALUE(token))
    {
      tok_str_add_record(copy, parameters->str, parameters->len, &i);
      continue;
    }
    for (parameter = 0; parameter < lambda->generic_param_count; ++parameter)
      if (token == lambda->generic_param_toks[parameter]
          && lambda->generic_param_is_pack[parameter])
      {
        int next = i + 1;
        while (next < parameters->len
               && parameters->str[next] == TOK_LINENUM)
          next += 1 + tok_str_value_extra_words(parameters->str,
                                                parameters->len, next);
        if (next < parameters->len && parameters->str[next] == TOK_DOTS)
        {
          tok_str_add(copy, token);
          i = next;
          break;
        }
      }
    if (parameter == lambda->generic_param_count)
      tok_str_add(copy, token);
  }
  return copy;
}

/* Parse the complete body for deduction. This uses the normal statement
   parser, so local declarations and shadowing determine capture use. */
static void probe_lambda_body(LambdaExpression *lambda, TokenString *parameters,
                              TokenString *body)
{
  struct scope isolated = {0};
  struct scope *saved_current = cur_scope, *saved_root = root_scope;
  struct scope *saved_loop = loop_scope;
  struct switch_t *saved_switch = cur_switch;
  struct temp_local_variable saved_temps[MAX_TEMP_LOCAL_VARIABLE_NUMBER];
  Sym *saved_locals, *saved_cleanups = all_cleanups, *saved_gotos = pending_gotos;
  Sym *saved_labels = local_label_stack, *scope_sym;
  Sym *saved_outer_locals = local_stack;
  SValue saved_values[VSTACK_SIZE];
  int saved_scope, saved_outer_scope = local_scope, i, saved_tok = tok, saved_loc = loc;
  int saved_nocode = nocode_wanted, saved_temp_count = nb_temp_local_vars;
  int saved_unevaluated_depth = cpp_unevaluated_expression_depth;
  int saved_defer = defer_pending_member_funcs;
  int saved_type_owner = local_type_owner_tok;
  int saved_type_index = local_type_declaration_index;
  CType saved_func_vt = func_vt;
  const char *saved_funcname = funcname;
  int value_count = (int)(vtop - vstack + 1);
  char saved_debug = debug_modes;
  CValue saved_tokc = tokc;
  TokenString macro = *body, *parent = macro_stack;
  TokenString *scope_parameters, *owned_scope_parameters = NULL;
  const int *parent_ptr = macro_ptr;
  LambdaExpression *saved_probe = lambda_probe;

  memcpy(saved_values, vstack, value_count * sizeof(SValue));
  memcpy(saved_temps, arr_temp_local_vars, sizeof(saved_temps));
  vtop = vstack - 1;
  nocode_wanted = 1;
  cpp_unevaluated_expression_depth = 0;
  defer_pending_member_funcs = 1;
  debug_modes = 0;
  nb_temp_local_vars = 0;
  cur_scope = root_scope = &isolated;
  func_vt = lambda->return_type;
  funcname = "lambda";
  cur_switch = NULL;
  loop_scope = NULL;
  all_cleanups = pending_gotos = local_label_stack = NULL;
  local_stack = NULL;
  local_scope = saved_outer_scope;
  if (lambda->generic)
    for (i = 0; i < lambda->generic_param_count; ++i)
    {
      CType generic_type = int_type;
      generic_type.t |= VT_TYPEDEF;
      sym_push(lambda->generic_param_toks[i], &generic_type, VT_CONST, 0);
    }
  scope_parameters = lambda_parameters_without_generic_packs(lambda, parameters);
  if (scope_parameters != parameters)
    owned_scope_parameters = scope_parameters;
  push_saved_param_scope(scope_parameters, &saved_locals, &saved_scope);
  /* Keep the parent symbols visible through the token table, but give the
     probe its own allocation stack and parameter scope. */
  for (i = 0; i < lambda->count; ++i)
  {
    CType visible = lambda->captures[i].type;
    if (!lambda->captures[i].by_reference && !lambda->mutable_call)
      visible.t |= VT_CONSTANT;
    sym_push(lambda->captures[i].name, &visible, VT_LOCAL | VT_LVAL, 0);
  }
  if (lambda->generic)
  {
    lambda->body_declaration_count = 0;
    for (scope_sym = local_stack; scope_sym; scope_sym = scope_sym->prev)
      if (scope_sym->v >= TOK_UIDENT
          && sym_scope_ex(scope_sym) > saved_outer_scope)
        lambda_note_body_declaration(lambda, scope_sym->v & ~SYM_FIELD, -1, -1);
  }
  lambda->parent_probe = saved_probe;
  if (lambda->body_owner_tok)
  {
    local_type_owner_tok = lambda->body_owner_tok;
    local_type_declaration_index = 0;
  }
  lambda_probe = lambda;
  if (lambda->generic)
    probe_generic_lambda_captures(lambda, body);
  else
  {
    begin_macro(&macro, 0);
    next();
    block(STMT_COMPOUND);
    while (macro_stack && macro_stack != parent) end_macro();
    macro_ptr = parent_ptr;
  }
  lambda_probe = saved_probe;
  local_type_owner_tok = saved_type_owner;
  local_type_declaration_index = saved_type_index;
  sym_pop(&local_stack, NULL, 0);
  sym_pop(&all_cleanups, NULL, 0);
  sym_pop(&pending_gotos, NULL, 0);
  label_pop(&local_label_stack, NULL, 0);
  if (saved_locals)
    sym_pop(&saved_locals, NULL, 0);
  local_stack = saved_outer_locals;
  local_scope = saved_outer_scope;
  all_cleanups = saved_cleanups;
  pending_gotos = saved_gotos;
  local_label_stack = saved_labels;
  cur_scope = saved_current;
  root_scope = saved_root;
  func_vt = saved_func_vt;
  funcname = saved_funcname;
  loop_scope = saved_loop;
  cur_switch = saved_switch;
  loc = saved_loc;
  nocode_wanted = saved_nocode;
  cpp_unevaluated_expression_depth = saved_unevaluated_depth;
  nb_temp_local_vars = saved_temp_count;
  defer_pending_member_funcs = saved_defer;
  debug_modes = saved_debug;
  memcpy(arr_temp_local_vars, saved_temps, sizeof(saved_temps));
  memcpy(vstack, saved_values, value_count * sizeof(SValue));
  vtop = vstack + value_count - 1;
  tok = saved_tok;
  tokc = saved_tokc;
  if (owned_scope_parameters)
    tok_str_free(owned_scope_parameters);
}
