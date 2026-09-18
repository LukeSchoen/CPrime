/* Substitution probes own their token replay and restore the caller's
   expression state on failure. Errors in instantiated class definitions are
   outside the immediate context and are not recovered here. */
static int template_probe_type_ex(TokenString *tokens, int argument,
                               int namespace_tok, int owner_tok, int function_tok, CType *result)
{
  struct ProbeState {
    TokenString *macro;
    TokenString replay;
    const int *ptr;
    CValue value;
    struct temp_local_variable temporaries[MAX_TEMP_LOCAL_VARIABLE_NUMBER];
    Sym *locals, *globals, *parameter_frame, *probe_globals;
    Sym **local_bindings, **global_bindings;
    CppMemberDeclContext *member_declaration_context;
    LambdaExpression *lambda;
    CppConstructionDestination *construction;
    int destination_available;
    ConstexprTemporaryObject *evaluation_temporaries;
    Sym *initialization_object;
    SValue initializer_receiver;
    int initializer_receiver_offset;
    int integral_fold_suppression;
    long long initialization_frontier;
    int temporary_expression_depth, initializer_probe_noeval;
    CType *member_address_target;
    jmp_buf *previous;
    int token, depth, nocode, local_offset, temporary_count, namespace_tok;
    int declarator_depth, alias_layout_depth;
    int global_expression;
    int member_class, declaration_auto, declaration_trailing_return;
    int declaration_typedef, declaration_decltype;
    int scope, deferred_members, binding_count, global_binding_count;
    int *query_alignment;
    int unevaluated_depth, probe_depth, standard_conversion_only;
    int permit_incomplete_array;
    int permit_deferred_bound;
    SValue values[1];
  } *saved = cprime_malloc(sizeof(*saved)
                           + (vtop - _vstack) * sizeof(SValue));
  jmp_buf recovery;
  volatile int success = 0;
  /* Mode 11 replays a member function template's substituted parameter list,
     so it uses the declaration-context setup of the probes below mode 6. */
  int declaration_context = argument < 6 || argument == 11;
  saved->macro = macro_stack; saved->ptr = macro_ptr;
  saved->replay = *tokens;
  saved->token = tok; saved->value = tokc;
  saved->depth = vtop - _vstack;
  /* Only the live expression stack belongs to the suspended caller. */
  memcpy(saved->values, _vstack, (saved->depth + 1) * sizeof(SValue));
  memcpy(saved->temporaries, arr_temp_local_vars, sizeof(saved->temporaries));
  saved->locals = local_stack;
  saved->globals = NULL; saved->parameter_frame = NULL;
  saved->local_bindings = NULL; saved->binding_count = 0;
  saved->global_bindings = NULL; saved->global_binding_count = 0;
  saved->scope = local_scope;
  saved->declarator_depth = cpp_member_declarator_depth;
  saved->alias_layout_depth = class_alias_layout_depth;
  saved->global_expression = global_expr;
  saved->deferred_members = defer_pending_member_funcs;
  saved->nocode = nocode_wanted; saved->local_offset = loc;
  saved->temporary_count = nb_temp_local_vars;
  saved->namespace_tok = current_namespace_scope_tok();
  saved->member_class = active_member_class_tok;
  saved->member_declaration_context = cpp_member_decl_context;
  saved->declaration_auto = last_decl_was_auto;
  saved->declaration_trailing_return = last_decl_had_trailing_return;
  saved->declaration_typedef = last_btype_was_typedef;
  saved->declaration_decltype = last_btype_was_decltype;
  saved->previous = cpp_substitution_jump;
  saved->query_alignment = active_type_query_alignment;
  active_type_query_alignment = NULL;
  saved->probe_depth = cpp_template_probe_depth++;
  saved->standard_conversion_only = cpp_standard_conversion_only;
  saved->unevaluated_depth = cpp_unevaluated_expression_depth;
  saved->permit_incomplete_array = cpp_permit_incomplete_array_type;
  saved->permit_deferred_bound = cpp_permit_deferred_array_bound;
  saved->lambda = lambda_probe;
  saved->construction = cpp_construction_destination;
  saved->destination_available = cpp_construction_destination
                                 ? cpp_construction_destination->available : 0;
  saved->evaluation_temporaries = constexpr_temporary_objects;
  saved->temporary_expression_depth = constexpr_temporary_expression_depth;
  saved->initializer_probe_noeval = constexpr_initializer_probe_noeval;
  saved->initialization_object = constant_initialization_object;
  saved->initializer_receiver = cpp_default_initializer_receiver;
  saved->initializer_receiver_offset = cpp_default_initializer_receiver_offset;
  saved->integral_fold_suppression = suppress_integral_constexpr_fold;
  saved->initialization_frontier = constant_initialization_frontier;
  saved->member_address_target = cpp_member_address_target;
  /* Modes 6/8 disambiguate expression type-ids and statement declarators in the current lexical
     context. Other probes use the template's declaration context. A caller's
     local typedef must not hide a name in that context. Substituted type
     arguments already have stable global spellings from template_subst_token. */
  if (saved->locals && declaration_context)
    saved->local_bindings = sym_hide_local_bindings(saved->locals,
                                                    &saved->binding_count);
  if (saved->scope && declaration_context)
  {
    Sym *frame, *next_binding;
    /* Types materialized inside a parameter declaration can add anonymous
       frames ahead of that declaration's own boundary. Hide all currently
       linked parameter-scope names, including the suspended outer frames. */
    for (frame = scoped_bindings; frame; frame = next_binding) {
      int name = frame->v & ~SYM_STRUCT;
      TokenSym *token;
      Sym *binding;
      next_binding = frame->scope_next;
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
    active_member_class_tok = declaration_context
                              ? owner_tok : saved->member_class;
    cpp_member_decl_context = declaration_context
                              ? NULL : saved->member_declaration_context;
    lambda_probe = NULL;
    ++nocode_wanted;
    cpp_permit_incomplete_array_type = argument == 6;
    /* A statement-declarator probe (mode 8) must accept VLA bounds from the
       surrounding function while deciding `T (*name)[bound];` versus a
       functional expression. The replay evaluates the bound in its real
       local scope. */
    cpp_permit_deferred_array_bound = argument == 6 || argument == 8;
    begin_macro(&saved->replay, 0);
    next();
    if (argument == 9)
    {
      nocode_wanted = 1;
      probe_static_aggregate_initializer(result);
      success = tok == TOK_EOF;
    }
    else if (argument == 7)
    {
      cpp_construction_destination = NULL;
      materialize_braced_temporary_mode(result, 1, NULL);
      success = tok == TOK_EOF;
      vpop();
    }
    else if (argument == 3)
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
      /* Initialization may target a const object; the constant-expression
         probe checks conversion, not assignment to an existing object. */
      target.t &= ~VT_CONSTANT;
      gen_assign_cast(&target);
      success = tok == TOK_EOF
          && (vtop->r & (VT_VALMASK | VT_LVAL | VT_NONCONST)) == VT_CONST;
      vpop();
    }
    else if (argument == 4 || argument == 5)
    {
      CType function;
      int name = 0;
      cpp_substitution_jump = NULL;
      if (parse_btype(&function, &attributes, 0)) {
        TokenString *body = NULL, *parameters = tok_str_alloc();
        LambdaExpression deduction;
        int saved_type_owner=local_type_owner_tok;
        int saved_type_index=local_type_declaration_index;
        Sym *parameter;
        type_decl(&function, &attributes, &name, TYPE_ABSTRACT);
        if ((function.t & VT_BTYPE) != VT_FUNC || tok != '{')
          cprime_error("saved function body expected");
        for (parameter = function.ref->next; parameter; parameter=parameter->next) {
          if (parameter != function.ref->next) tok_str_add(parameters, ',');
          add_ctype_declarator_tokens(parameters, &parameter->type, parameter->v & ~SYM_FIELD);
        }
        tok_str_add(parameters, TOK_EOF);
        skip_or_save_block(&body);
        tok_str_add(body, TOK_EOF);
        memset(&deduction, 0, sizeof(deduction));
        deduction.return_type.t = VT_VOID;
        deduction.return_deduction_kind = argument == 5 ? 2 : 1;
        deduction.return_pattern = function.ref->type;
        deduction.mutable_call = 1;
        deduction.owner = function_tok;
        local_type_owner_tok = function_tok;
        local_type_declaration_index = 0;
        active_member_class_tok = owner_tok;
        cpp_substitution_jump = NULL;
        probe_lambda_body(&deduction, parameters, body);
        if (!deduction.return_seen && deduction.return_deduction_kind == 1
            && (deduction.return_pattern.t & VT_BTYPE) == VT_PTR)
          cprime_error("cannot deduce auto pointer or reference without a return expression");
        cpp_substitution_jump = &recovery;
        *result = deduction.return_type;
        local_type_owner_tok = saved_type_owner;
        local_type_declaration_index = saved_type_index;
        tok_str_free(parameters); tok_str_free(body);
        success = 1;
      }
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
    else if (argument == 11)
    {
      /* The substituted parameter list of a member function template. Build
         its prototype in the declaration's own context; a template argument
         that cannot match a parameter's value type is a substitution failure,
         so the enclosing call candidate is dropped instead of hard-erroring.
         The caller's return type arrives in *result and the prototype leaves
         through the same place. */
      CType ret_type = *result;
      *result = make_member_func_type_from_saved_params(&ret_type, tokens,
                                                        owner_tok);
      success = 1;
    }
    else if (argument && argument < 6)
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
      type_decl(result, &attributes, &declarator,
                argument == 8 ? TYPE_DIRECT : TYPE_ABSTRACT);
      success = tok == TOK_EOF;
    }
  }
  cpp_substitution_jump = saved->previous;
  active_type_query_alignment = saved->query_alignment;
  cpp_template_probe_depth = saved->probe_depth;
  cpp_standard_conversion_only = saved->standard_conversion_only;
  cpp_unevaluated_expression_depth = saved->unevaluated_depth;
  cpp_permit_incomplete_array_type = saved->permit_incomplete_array;
  cpp_permit_deferred_array_bound = saved->permit_deferred_bound;
  lambda_probe = saved->lambda;
  cpp_construction_destination = saved->construction;
  /* A probe may consume the caller's pending construction destination.  The
     probe owns no object, so hand the destination back to the caller exactly
     as it was handed in. */
  if (cpp_construction_destination)
    cpp_construction_destination->available = saved->destination_available;
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
  cpp_member_declarator_depth = saved->declarator_depth;
  /* A recovered replay can abandon a declarator that still owned an indirect
     class alias layout.  Leaving the counter raised makes every later
     specialization publish only a forward type, so its members go missing. */
  class_alias_layout_depth = saved->alias_layout_depth;
  cpp_member_address_target = saved->member_address_target;
  global_expr = saved->global_expression;
  defer_pending_member_funcs = saved->deferred_members;
  release_constexpr_temporary_objects(saved->evaluation_temporaries, 0);
  constexpr_temporary_expression_depth = saved->temporary_expression_depth;
  constexpr_initializer_probe_noeval = saved->initializer_probe_noeval;
  constant_initialization_object = saved->initialization_object;
  cpp_default_initializer_receiver = saved->initializer_receiver;
  cpp_default_initializer_receiver_offset = saved->initializer_receiver_offset;
  suppress_integral_constexpr_fold = saved->integral_fold_suppression;
  constant_initialization_frontier = saved->initialization_frontier;
  memcpy(_vstack, saved->values, (saved->depth + 1) * sizeof(SValue));
  vtop = _vstack + saved->depth;
  memcpy(arr_temp_local_vars, saved->temporaries, sizeof(saved->temporaries));
  nb_temp_local_vars = saved->temporary_count;
  loc = saved->local_offset; nocode_wanted = saved->nocode;
  enter_namespace_scope(saved->namespace_tok);
  active_member_class_tok = saved->member_class;
  cpp_member_decl_context = saved->member_declaration_context;
  last_decl_was_auto = saved->declaration_auto;
  last_decl_had_trailing_return = saved->declaration_trailing_return;
  last_btype_was_typedef = saved->declaration_typedef;
  last_btype_was_decltype = saved->declaration_decltype;
  cprime_free(saved);
  return success;
}

static int template_probe_type(TokenString *tokens, int argument,
                               int namespace_tok, CType *result)
{
  return template_probe_type_ex(tokens, argument, namespace_tok, 0, 0, result);
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
    /* `>>` at one open angle closes this argument's template-id and the
       enclosing template-id together (`Helper<T, void_t<T &>>`).  Keep the
       first '>' for the argument being read and leave the second for the
       enclosing list, which otherwise sees a single shift token and never
       finishes the argument. */
    if (!parentheses && !brackets && tok == TOK_SAR && angles == 1)
    {
      tok_str_add(tokens, TOK_GT);
      tok = TOK_GT;
      unget_tok(TOK_GT);
      break;
    }
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
  /* A non-type pack (`size_t... Indices`) is spelled by name, so its elements
     in a parameter type and in the trailing return type are values rather
     than types, and the parameter list needs no per-element declarator. */
  int pack_is_value = pack >= 0 && mask_test(&definition->value_param_mask, pack);
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
    int parens = 0, brackets = 0, angles = 0, has_pack = 0, pack_in_type = 0;
    int element, count = 1;
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
      if (token == TOK_DOTS && pack_type) {
        has_pack = 1;
        /* A pack inside the parameter's type (`integer_sequence<size_t, I...>`)
           declares one parameter whose type lists every element, while a
           function parameter pack (`T &&...values`) declares one parameter per
           element. */
        if (parens || brackets || angles) pack_in_type = 1;
      }
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
    if (has_pack && pack_in_type) {
      TokenString *parameter = tok_str_alloc(), *expanded_parameter;
      if (header->str[header->len - 1] != '(') tok_str_add(header, ',');
      for (j = start; j < end; ++j)
        tok_str_add_record(parameter, source->str, source->len, &j);
      tok_str_add(parameter, TOK_EOF);
      expanded_parameter = expand_template_expression_packs(parameter,
          pack_type, 0, 0, bindings->toks + pack, count);
      for (j = 0; j < expanded_parameter->len; ++j) {
        int token = expanded_parameter->str[j];
        if (token == TOK_EOF) break;
        if (TOK_HAS_VALUE(token))
          tok_str_add_record(header, expanded_parameter->str,
                             expanded_parameter->len, &j);
        else tok_str_add(header, template_subst_token(definition, bindings, 0,
                                                      NULL, NULL, 0, token));
      }
      if (expanded_parameter != parameter) tok_str_free(expanded_parameter);
      tok_str_free(parameter);
    }
    else for (element = 0; element < count; ++element) {
      char name[256];
      if (header->str[header->len - 1] != '(') tok_str_add(header, ',');
      for (j = start; j < end; ++j) {
        int token = source->str[j];
        if (has_pack && token == TOK_DOTS) continue;
        if (has_pack && token == pack_type)
          tok_str_add(header, bindings->toks[pack + element]);
        else if (has_pack && token == pack_name && element && !pack_is_value) {
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
  if (pack_name && !pack_is_value)
    expanded = expand_template_expression_packs(tail, pack_type, pack_name,
        pack_name, bindings->toks + pack, bindings->nb - pack);
  else if (pack_is_value)
    expanded = expand_template_expression_packs(tail, pack_type, 0, 0,
        bindings->toks + pack, bindings->nb - pack);
  else
    expanded = tail;
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
    else tok_str_add(tokens, token);
  }
  tok_str_add(tokens, TOK_EOF);
  /* A return type that names the pack (`tuple<decay<Ts>::type...>`) must be
     expanded while the pack spelling is still present. Substituting first
     would replace the pack with its first argument and leave the ellipsis
     behind, which then fails to parse as a type. */
  if (definition->variadic_param_index >= 0)
  {
    int pack = definition->variadic_param_index;
    TokenString *expanded = expand_template_expression_packs(tokens,
        definition->type_param_toks[pack], 0, 0,
        bindings->toks + pack, bindings->nb - pack);
    tok_str_free(tokens);
    tokens = expanded;
  }
  {
    TokenString *substituted = tok_str_alloc();
    for (i = 0; i < tokens->len; ++i)
    {
      int token = tokens->str[i];
      if (token == TOK_EOF) break;
      if (TOK_HAS_VALUE(token))
      {
        tok_str_add_record(substituted, tokens->str, tokens->len, &i);
        continue;
      }
      tok_str_add(substituted, template_subst_token(definition, bindings, 0,
                                                    NULL, NULL, 0, token));
    }
    tok_str_add(substituted, TOK_EOF);
    tok_str_free(tokens);
    tokens = substituted;
  }
  valid = template_probe_type(tokens, 0, definition->namespace_tok, &result);
  tok_str_free(tokens);
  return valid;
}
