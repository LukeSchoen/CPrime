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
    Sym *locals, *globals, *parameter_frame, *probe_globals;
    Sym **local_bindings, **global_bindings;
    CppMemberDeclContext *member_declaration_context;
    LambdaExpression *lambda;
    jmp_buf *previous;
    int token, depth, nocode, local_offset, temporary_count, namespace_tok;
    int member_class, declaration_auto, declaration_typedef, declaration_decltype;
    int scope, deferred_members, binding_count, global_binding_count;
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
  saved->global_bindings = NULL; saved->global_binding_count = 0;
  saved->scope = local_scope;
  saved->deferred_members = defer_pending_member_funcs;
  saved->nocode = nocode_wanted; saved->local_offset = loc;
  saved->temporary_count = nb_temp_local_vars;
  saved->namespace_tok = current_namespace_scope_tok();
  saved->member_class = active_member_class_tok;
  saved->member_declaration_context = cpp_member_decl_context;
  saved->declaration_auto = last_decl_was_auto;
  saved->declaration_typedef = last_btype_was_typedef;
  saved->declaration_decltype = last_btype_was_decltype;
  saved->previous = cpp_substitution_jump;
  saved->lambda = lambda_probe;
  /* Lookup belongs to the template's declaration context. Even a caller's
     local typedef must not hide a name in that context. Substituted type
     arguments already have stable global spellings from template_subst_token. */
  if (saved->locals)
    saved->local_bindings = sym_hide_local_bindings(saved->locals,
                                                    &saved->binding_count);
  if (saved->scope)
  {
    Sym *frame;
    /* Types materialized inside a parameter declaration can add anonymous
       frames ahead of that declaration's own boundary. Hide all currently
       linked parameter-scope names, including the suspended outer frames. */
    for (frame = global_stack; frame; frame = frame->prev) {
      int name = frame->v & ~SYM_STRUCT;
      TokenSym *token;
      Sym *binding;
      if (name < TOK_IDENT || name >= SYM_FIRST_ANOM || sym_scope_ex(frame) <= 0)
        continue;
      token = table_ident[name - TOK_IDENT];
      binding = frame->v & SYM_STRUCT ? token->sym_struct : token->sym_identifier;
      if (binding != frame) continue;
      dynarray_add(&saved->global_bindings, &saved->global_binding_count, frame);
      sym_link(frame, 0);
    }
    for (frame = saved->locals ? NULL : global_stack; frame; frame = frame->prev)
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
  saved->probe_globals = global_stack;
  defer_pending_member_funcs = 1;
  vtop = vstack - 1;
  cpp_substitution_jump = &recovery;
  if (!setjmp(recovery))
  {
    AttributeDef attributes;
    enter_namespace_scope(namespace_tok);
    active_member_class_tok = 0;
    cpp_member_decl_context = NULL;
    lambda_probe = NULL;
    ++nocode_wanted;
    begin_macro(&saved->replay, 0);
    next();
    if (argument == 3)
    {
      CType target = *result;
      int braced = tok == '{';
      if (braced) next();
      nocode_wanted += CONST_WANTED_BIT;
      if (braced && tok == '}') vpushi(0);
      else expr_eq();
      nocode_wanted -= CONST_WANTED_BIT;
      if (braced) {
        if (tok == ',') next();
        skip('}');
      }
      if (is_reference_type(&target)) {
        decay_reference_type(&target);
        if (is_reference_type(&vtop->type)) decay_reference_type(&vtop->type);
        else { test_lvalue(); mk_pointer(&vtop->type); gaddrof(); }
      }
      gen_assign_cast(&target);
      success = tok == TOK_EOF
          && (vtop->r & (VT_VALMASK | VT_LVAL | VT_NONCONST)) == VT_CONST;
      vpop();
    }
    else if (argument == 2)
    {
      /* Only generated declarations reach this mode. Their interfaces may
         be needed while the outer worklist is parsing another signature;
         publishing them must not recursively drain function bodies. */
      while (tok != TOK_EOF && tok != ';')
        decl(VT_CONST);
      success = 1;
    }
    else if (argument)
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
  lambda_probe = saved->lambda;
  while (macro_stack && macro_stack != saved->macro) end_macro();
  macro_ptr = saved->ptr; tok = saved->token; tokc = saved->value;
  /* A failed nested replay can abandon its own local stack. The caller's
     stack is detached, so cleanup never depends on it remaining an ancestor. */
  sym_pop(&local_stack, NULL, 0);
  {
    Sym *symbol;
    /* An error may leave a parameter declarator on the global symbol arena.
       Keep its retained type storage, but end every newly entered local
       binding before restoring the suspended declaration's parameter frame. */
    for (symbol = global_stack; symbol && symbol != saved->probe_globals;
         symbol = symbol->prev)
      if ((symbol->v & ~SYM_STRUCT) >= TOK_IDENT
          && (symbol->v & ~SYM_STRUCT) < SYM_FIRST_ANOM
          && sym_scope_ex(symbol) > 0)
        sym_link(symbol, 0);
  }
  if (saved->parameter_frame)
  {
    saved->parameter_frame->prev = global_stack;
    global_stack = saved->globals;
  }
  sym_relink_stack(saved->global_bindings, saved->global_binding_count);
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
  cpp_member_decl_context = saved->member_declaration_context;
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

/* Form an abstract function declarator so post_type() installs the actual
   parameter names while substituting a dependent trailing return type. */
static int template_trailing_return_type(TemplateDef *definition,
                                         TemplateArgList *bindings,
                                         int name_index, CType *result)
{
  TokenString *header, *tail, *expanded;
  TokenString *source = definition->def_str;
  int i, j, start, end, close = -1, depth = 0, valid;
  int pack = definition->variadic_param_index;
  int pack_type = pack >= 0 ? definition->type_param_toks[pack] : 0;
  int pack_name = 0;
  for (i = name_index + 1; i < source->len; ++i) {
    int token = source->str[i];
    if (TOK_HAS_VALUE(token)) {
      i += tok_str_value_extra_words(source->str, source->len, i);
      continue;
    }
    if (token == '(') ++depth;
    else if (token == ')' && !--depth) { close = i; break; }
  }
  if (close < 0) return -1;
  for (i = close + 1; i < source->len; ++i) {
    int token = source->str[i];
    if (token == TOK_ARROW) break;
    if (token == '{' || token == ';' || token == TOK_EOF) return -1;
    if (TOK_HAS_VALUE(token))
      i += tok_str_value_extra_words(source->str, source->len, i);
  }
  if (i == source->len) return -1;
  header = tok_str_alloc();
  for (i = 0; i < name_index; ++i) {
    int token = source->str[i];
    if (token == TOK_STATIC || token == TOK_EXTERN || token == TOK_INLINE1
        || token == TOK_INLINE2 || token == TOK_INLINE3 || token == tok_constexpr)
      continue;
    if (TOK_HAS_VALUE(token))
      tok_str_add_record(header, source->str, source->len, &i);
    else tok_str_add(header, template_subst_token(definition, bindings, 0,
                                                   NULL, NULL, 0, token));
  }
  tok_str_add(header, '(');
  start = name_index + 2;
  while (start < close) {
    int parens = 0, brackets = 0, angles = 0, has_pack = 0, element, count = 1;
    for (end = start; end < close; ++end) {
      int token = source->str[end];
      if (TOK_HAS_VALUE(token)) {
        end += tok_str_value_extra_words(source->str, source->len, end);
        continue;
      }
      if (!parens && !brackets && !angles && token == ',') break;
      if (token == '(') ++parens;
      else if (token == ')') --parens;
      else if (token == '[') ++brackets;
      else if (token == ']') --brackets;
      else if (!parens && !brackets && token == TOK_LT) ++angles;
      else if (!parens && !brackets && token == TOK_GT) --angles;
      else if (!parens && !brackets && token == TOK_SAR) angles -= 2;
      if (token == TOK_DOTS && pack_type) has_pack = 1;
    }
    if (has_pack) {
      count = bindings->nb - pack;
      for (j = start; j < end; ++j) {
        int token = source->str[j];
        if (TOK_HAS_VALUE(token)) {
          j += tok_str_value_extra_words(source->str, source->len, j);
          continue;
        }
        if (token >= TOK_UIDENT && token != pack_type
            && template_param_index(definition, token) < 0)
          pack_name = token;
      }
    }
    for (element = 0; element < count; ++element) {
      char name[256];
      if (header->str[header->len - 1] != '(') tok_str_add(header, ',');
      for (j = start; j < end; ++j) {
        int token = source->str[j];
        if (has_pack && token == TOK_DOTS) continue;
        if (has_pack && token == pack_type)
          tok_str_add(header, bindings->toks[pack + element]);
        else if (has_pack && token == pack_name && element) {
          snprintf(name, sizeof(name), "%s__pack%d", get_tok_str(pack_name, NULL), element);
          tok_str_add(header, tok_alloc_const(name));
        }
        else if (TOK_HAS_VALUE(token))
          tok_str_add_record(header, source->str, source->len, &j);
        else tok_str_add(header, template_subst_token(definition, bindings, 0,
                                                       NULL, NULL, 0, token));
      }
    }
    start = end + 1;
  }
  tok_str_add(header, ')');
  tail = tok_str_alloc();
  depth = 0;
  for (i = close + 1; i < source->len; ++i) {
    int token = source->str[i];
    if (!depth && (token == '{' || token == ';' || token == TOK_EOF)) break;
    if (token == '(') ++depth;
    else if (token == ')') --depth;
    tok_str_add_record(tail, source->str, source->len, &i);
  }
  tok_str_add(tail, TOK_EOF);
  expanded = pack_name ? expand_template_expression_packs(tail, pack_type,
      pack_name, pack_name, bindings->toks + pack, bindings->nb - pack) : tail;
  for (i = 0; i < expanded->len; ++i) {
    int token = expanded->str[i];
    if (token == TOK_EOF) break;
    if (TOK_HAS_VALUE(token))
      tok_str_add_record(header, expanded->str, expanded->len, &i);
    else tok_str_add(header, template_subst_token(definition, bindings, 0,
                                                   NULL, NULL, 0, token));
  }
  tok_str_add(header, TOK_EOF);
  valid = template_probe_type(header, 0, definition->namespace_tok, result);
  tok_str_free(header);
  if (expanded != tail) tok_str_free(expanded);
  tok_str_free(tail);
  return valid;
}

static int template_return_substitution_matches_bound(TemplateDef *definition,
                                                TemplateArgList *bindings)
{
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
  if (end < 0) return 1;
  valid = template_trailing_return_type(definition, bindings, end, &result);
  if (valid >= 0) return valid;
  if (!dependent) return 1;
  tokens = tok_str_alloc();
  for (i = 0; i < end; ++i)
  {
    int token = definition->def_str->str[i];
    if (token == TOK_INLINE1 || token == TOK_INLINE2 || token == TOK_INLINE3
        || token == TOK_STATIC || token == TOK_EXTERN || token == tok_constexpr)
      continue;
    if (TOK_HAS_VALUE(token))
      tok_str_add_record(tokens, definition->def_str->str, definition->def_str->len, &i);
    else tok_str_add(tokens, template_subst_token(definition, bindings, 0,
                                                   NULL, NULL, 0, token));
  }
  tok_str_add(tokens, TOK_EOF);
  valid = template_probe_type(tokens, 0, definition->namespace_tok, &result);
  tok_str_free(tokens);
  return valid;
}

static int template_return_substitution_matches(TemplateDef *definition,
                                                CType *arguments, int count)
{
  TemplateArgList bindings;
  if (!infer_template_args_from_call(definition, arguments, count, &bindings))
    return 0;
  return template_return_substitution_matches_bound(definition, &bindings);
}
