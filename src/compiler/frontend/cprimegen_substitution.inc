/* Substitution probes own their token replay and restore the caller's
   expression state on failure. Errors in instantiated class definitions are
   outside the immediate context and are not recovered here. */
static int template_probe_type(TokenString *tokens, int argument,
                               int namespace_tok, CType *result)
{
  struct ProbeState {
    TokenString *macro;
    TokenString replay;
    const int *ptr;
    CValue value;
    SValue values[1 + VSTACK_SIZE];
    struct temp_local_variable temporaries[MAX_TEMP_LOCAL_VARIABLE_NUMBER];
    Sym *locals, *globals, *parameter_frame;
    Sym **local_bindings;
    jmp_buf *previous;
    int token, depth, nocode, local_offset, temporary_count, namespace_tok;
    int member_class, declaration_auto, declaration_typedef, declaration_decltype;
    int scope, deferred_members, binding_count;
  } *saved = cprime_malloc(sizeof(*saved));
  jmp_buf recovery;
  volatile int success = 0;
  saved->macro = macro_stack; saved->ptr = macro_ptr;
  saved->replay = *tokens;
  saved->token = tok; saved->value = tokc;
  saved->depth = vtop - _vstack;
  memcpy(saved->values, _vstack, sizeof(saved->values));
  memcpy(saved->temporaries, arr_temp_local_vars, sizeof(saved->temporaries));
  saved->locals = local_stack;
  saved->globals = NULL; saved->parameter_frame = NULL;
  saved->local_bindings = NULL; saved->binding_count = 0;
  saved->scope = local_scope;
  saved->deferred_members = defer_pending_member_funcs;
  saved->nocode = nocode_wanted; saved->local_offset = loc;
  saved->temporary_count = nb_temp_local_vars;
  saved->namespace_tok = current_namespace_scope_tok();
  saved->member_class = active_member_class_tok;
  saved->declaration_auto = last_decl_was_auto;
  saved->declaration_typedef = last_btype_was_typedef;
  saved->declaration_decltype = last_btype_was_decltype;
  saved->previous = cpp_substitution_jump;
  /* Lookup belongs to the template's declaration context. Even a caller's
     local typedef must not hide a name in that context. Substituted type
     arguments already have stable global spellings from template_subst_token. */
  if (saved->locals)
    saved->local_bindings = sym_hide_local_bindings(saved->locals,
                                                    &saved->binding_count);
  else if (saved->scope)
  {
    Sym *frame;
    for (frame = global_stack; frame; frame = frame->prev)
      if (frame->v == SYM_FIELD && frame->sym_scope == 0)
      {
        saved->globals = global_stack;
        saved->parameter_frame = frame;
        global_stack = frame->prev;
        break;
      }
  }
  local_stack = NULL;
  local_scope = 0;
  defer_pending_member_funcs = 1;
  vtop = vstack - 1;
  cpp_substitution_jump = &recovery;
  if (!setjmp(recovery))
  {
    AttributeDef attributes;
    enter_namespace_scope(namespace_tok);
    active_member_class_tok = 0;
    ++nocode_wanted;
    begin_macro(&saved->replay, 0);
    next();
    if (argument)
    {
      int token = parse_one_template_type_arg();
      if (token && tok == TOK_EOF)
      {
        result->t = token;
        result->ref = NULL;
        success = 1;
      }
    }
    else if (parse_btype(result, &attributes, 0))
    {
      int declarator = 0;
      type_decl(result, &attributes, &declarator, TYPE_ABSTRACT);
      success = tok == TOK_EOF;
    }
  }
  cpp_substitution_jump = saved->previous;
  while (macro_stack && macro_stack != saved->macro) end_macro();
  macro_ptr = saved->ptr; tok = saved->token; tokc = saved->value;
  /* A failed nested replay can abandon its own local stack. The caller's
     stack is detached, so cleanup never depends on it remaining an ancestor. */
  sym_pop(&local_stack, NULL, 0);
  if (saved->parameter_frame)
  {
    saved->parameter_frame->prev = global_stack;
    global_stack = saved->globals;
  }
  sym_relink_stack(saved->local_bindings, saved->binding_count);
  local_stack = saved->locals;
  local_scope = saved->scope;
  defer_pending_member_funcs = saved->deferred_members;
  memcpy(_vstack, saved->values, sizeof(saved->values));
  vtop = _vstack + saved->depth;
  memcpy(arr_temp_local_vars, saved->temporaries, sizeof(saved->temporaries));
  nb_temp_local_vars = saved->temporary_count;
  loc = saved->local_offset; nocode_wanted = saved->nocode;
  enter_namespace_scope(saved->namespace_tok);
  active_member_class_tok = saved->member_class;
  last_decl_was_auto = saved->declaration_auto;
  last_btype_was_typedef = saved->declaration_typedef;
  last_btype_was_decltype = saved->declaration_decltype;
  cprime_free(saved);
  return success;
}

static int template_substituted_pattern_argument(TemplateDef *definition,
                                                 TemplateArgList *bindings)
{
  TokenString *tokens = tok_str_alloc();
  CType parsed;
  int parentheses = 0, brackets = 0, angles = 0, valid;
  while (tok != TOK_EOF)
  {
    if (!parentheses && !brackets && !angles
        && (tok == ',' || tok == TOK_GT || tok == TOK_SAR)) break;
    if (tok == '(') ++parentheses;
    else if (tok == ')') --parentheses;
    else if (tok == '[') ++brackets;
    else if (tok == ']') --brackets;
    else if (!parentheses && !brackets)
    {
      if (tok == TOK_LT) ++angles;
      else if (tok == TOK_GT) --angles;
      else if (tok == TOK_SAR) angles -= 2;
    }
    if (TOK_HAS_VALUE(tok)) tok_str_add_tok(tokens);
    else tok_str_add(tokens, template_subst_token(definition, bindings, 0,
                                                   NULL, NULL, 0, tok));
    next();
  }
  tok_str_add(tokens, TOK_EOF);
  valid = template_probe_type(tokens, 1, definition->namespace_tok, &parsed);
  tok_str_free(tokens);
  return valid ? parsed.t : 0;
}

static int template_return_substitution_matches(TemplateDef *definition,
                                                CType *arguments, int count)
{
  TemplateArgList bindings;
  TokenString *tokens;
  CType result;
  int i, end = -1, dependent = 0, valid;
  for (i = 0; i + 1 < definition->def_str->len; ++i)
  {
    int token = definition->def_str->str[i];
    if (TOK_HAS_VALUE(token))
    {
      i += tok_str_value_extra_words(definition->def_str->str,
                                      definition->def_str->len, i);
      continue;
    }
    if (token == definition->name_tok && definition->def_str->str[i + 1] == '(')
    { end = i; break; }
    if (template_param_index(definition, token) >= 0) dependent = 1;
  }
  if (!dependent || end < 0) return 1;
  if (!infer_template_args_from_call(definition, arguments, count, &bindings))
    return 0;
  tokens = tok_str_alloc();
  for (i = 0; i < end; ++i)
  {
    int token = definition->def_str->str[i];
    if (token == TOK_INLINE1 || token == TOK_INLINE2 || token == TOK_INLINE3
        || token == TOK_STATIC || token == TOK_EXTERN || token == tok_constexpr)
      continue;
    if (TOK_HAS_VALUE(token))
      tok_str_add_record(tokens, definition->def_str->str, definition->def_str->len, &i);
    else tok_str_add(tokens, template_subst_token(definition, &bindings, 0,
                                                   NULL, NULL, 0, token));
  }
  tok_str_add(tokens, TOK_EOF);
  valid = template_probe_type(tokens, 0, definition->namespace_tok, &result);
  tok_str_free(tokens);
  return valid;
}
