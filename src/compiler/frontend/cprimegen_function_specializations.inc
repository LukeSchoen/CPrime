static int compare_free_function_template_order(TemplateDef *, TemplateDef *);
static int infer_template_args_from_call_with_explicit(TemplateDef *, CType *, int,
                                                       TemplateArgList *, TemplateArgList *);
/* A qualified friend declaration can name a deduced specialization without
   an explicit template argument list. It must not introduce a new overload. */
static int bind_qualified_friend_template(int name, CType *function)
{
  CType actual[16];
  Sym *parameter;
  TemplateDef *candidate;
  TemplateDef *selected_template = NULL;
  FreeFuncOverload *ordinary;
  int count = 0, selected = 0;
  for (ordinary = free_func_candidates(name); ordinary; ordinary = ordinary->name_next)
    if (ordinary->name_tok == name
        && same_func_param_signature(&ordinary->func_type, function)
        && compare_types(&ordinary->func_type.ref->type, &function->ref->type, 1))
      return 1;
  for (parameter = function->ref->next; parameter; parameter = parameter->next) {
    if (count == 16) return 0;
    actual[count] = parameter->type;
    if (is_reference_type(&actual[count])) {
      int category = actual[count].t & (VT_REFERENCE | VT_RVALUE_REFERENCE);
      actual[count] = *pointed_type(&actual[count]);
      actual[count].t |= category;
    }
    ++count;
  }
  for (candidate = template_def_candidates(name); candidate;
       candidate = candidate->lookup_bucket_next) {
    TemplateArgList arguments;
    Sym *symbol;
    int instantiated;
    if (candidate->is_class || candidate->is_alias
        || template_def_lookup_tok(candidate) != name
        || candidate->func_max_args != count) continue;
    if (!infer_template_args_from_call_with_explicit(candidate, actual, count,
                                                    &arguments, NULL)) continue;
    instantiated = instantiate_template_if_needed(candidate, &arguments);
    compile_pending_template_specs_without_member_flush();
    symbol = global_symbol_find(instantiated);
    if (!symbol || !same_func_param_signature(&symbol->type, function)
        || !compare_types(&symbol->type.ref->type, &function->ref->type, 1)) continue;
    if (selected && selected != instantiated) {
      int order = compare_free_function_template_order(candidate, selected_template);
      if (order < 0) continue;
      if (!order) cprime_error("ambiguous qualified friend function");
    }
    selected = instantiated;
    selected_template = candidate;
  }
  return selected != 0;
}

/* Bind an explicit function specialization through its declaration. A '<'
   inside the body is never part of the specialization's declarator. */
static int explicit_lifecycle_declarator(void)
{
  TokenString *prefix = tok_str_alloc();
  int angle = 0, parenthesis = 0, previous = 0, qualifier = 0;
  int name = 0, destructor = 0, result = 0;
  while (tok != TOK_EOF && tok != ';' && tok != '{') {
    if (!angle && !parenthesis && tok == '(') {
      if (qualifier && name == qualifier) result = destructor ? 2 : 1;
      break;
    }
    if (!angle && !parenthesis) {
      if (tok >= TOK_UIDENT) name = tok;
      else if (tok == ':' && previous == ':') qualifier = name;
      else if (tok == '~') destructor = 1;
    }
    if (tok == '(') ++parenthesis;
    else if (tok == ')' && parenthesis) --parenthesis;
    else if (!parenthesis && tok == TOK_LT) ++angle;
    else if (!parenthesis && tok == TOK_GT && angle) --angle;
    else if (!parenthesis && tok == TOK_SAR && angle) angle = angle > 1 ? angle - 2 : 0;
    previous = tok;
    tok_str_add_tok(prefix); next();
  }
  restore_cpp_lifecycle_probe(prefix);
  return result;
}

static void parse_explicit_function_specialization(int instantiation)
{
  TokenString *parameters = tok_str_alloc(), *definition = tok_str_alloc();
  CType result, function, actual[16];
  AttributeDef attributes;
  TemplateArgList explicit_args = {{0}, 0};
  int name, qualified, owner = 0, qualifier = 0, cv, exception_spec;
  int count = 0, depth = 0, function_tok = 0, i, saved_tok, is_inline;
  int saved_class_depth = nb_defining_class_stack;
  int saved_namespace = current_namespace_scope_tok();
  CValue saved_tokc;
  Sym *parameter, *symbol;
  TemplateDef *selected_free = NULL;
  int lifecycle = explicit_lifecycle_declarator();

  if (lifecycle) {
    memset(&attributes, 0, sizeof(attributes));
    result.t = VT_VOID; result.ref = NULL;
  } else if (!parse_btype(&result, &attributes, 0)) expect("specialization return type");
  is_inline = !!(result.t & VT_INLINE);
  while (tok == '*' || tok == '&' || tok == TOK_LAND) {
    int reference = tok == '&' ? VT_REFERENCE : tok == TOK_LAND ? VT_RVALUE_REFERENCE : 0;
    mk_pointer(&result); result.t |= reference;
    next();
    while (tok == TOK_CONST1 || tok == TOK_VOLATILE1) {
      result.t |= tok == TOK_CONST1 ? VT_CONSTANT : VT_VOLATILE;
      next();
    }
  }
  for (;;) {
    TemplateArgList component_args = {{0}, 0};
    int has_args = 0;
    if (lifecycle == 2 && tok == '~') next();
    if (tok == TOK_OPERATOR) {
      name = parse_cpp_operator_method_tok();
      unget_tok(name);
    } else if (tok < TOK_UIDENT) expect("specialization name");
    name = tok;
    if (qualifier && is_namespace_tok(qualifier)) {
      int parts[2] = { qualifier, name };
      qualified = make_namespace_tok_from_parts(parts, 2);
    } else qualified = qualifier ? make_static_member_tok(qualifier, name)
                                 : find_current_namespace_tok(name);
    next();
    if (tok == TOK_LT || tok == '<') {
      has_args = 1;
      parse_template_type_args(&component_args);
    }
    if (tok != ':') {
      explicit_args = component_args;
      break;
    }
    next(); skip(':');
    if (has_args) {
      TemplateDef *record = find_class_template_def(qualified);
      if (!record) cprime_error("class template expected in specialization qualifier");
      qualified = instantiate_template_if_needed(record, &component_args);
      compile_pending_template_specs_without_member_flush();
    }
    qualifier = qualified;
    if (struct_find(qualifier)) owner = qualifier;
  }
  if (lifecycle) {
    if (!owner || !class_tok_matches_unqualified_name(owner, name))
      cprime_error("lifecycle specialization requires its class name");
    name = lifecycle == 1 ? TOK_CONSTRUCTOR1 : TOK_DESTRUCTOR1;
  }
  if (owner && tok != '(' && !instantiation && !explicit_args.nb) {
    int braces = 0, parentheses = 0, brackets = 0;
    Sym *member = global_symbol_find(make_static_member_tok(owner, name));
    if (!member || (member->type.t & VT_BTYPE) == VT_FUNC)
      cprime_error("specialization requires a declared static data member");
    add_ctype_tokens(definition, &result);
    tok_str_add(definition, owner);
    tok_str_add(definition, ':'); tok_str_add(definition, ':');
    tok_str_add(definition, name);
    while (tok != TOK_EOF) {
      int finished = tok == ';' && !braces && !parentheses && !brackets;
      if (tok == '{') ++braces;
      else if (tok == '}') --braces;
      else if (tok == '(') ++parentheses;
      else if (tok == ')') --parentheses;
      else if (tok == '[') ++brackets;
      else if (tok == ']') --brackets;
      tok_str_add_tok(definition); next();
      if (finished) break;
    }
    tok_str_add(definition, TOK_EOF);
    saved_tok = tok; saved_tokc = tokc;
    enter_namespace_scope(symbol_namespace_scope_tok(owner));
    begin_macro(definition, 1); next();
    decl(VT_CONST);
    end_macro(); tok = saved_tok; tokc = saved_tokc;
    enter_namespace_scope(saved_namespace);
    tok_str_free(parameters);
    return;
  }
  skip('(');
  while (tok != TOK_EOF && (tok != ')' || depth)) {
    if (tok == '(') ++depth;
    else if (tok == ')') --depth;
    tok_str_add_tok(parameters); next();
  }
  skip(')'); tok_str_add(parameters, TOK_EOF);
  cv = skip_member_func_cv_qualifiers();
  exception_spec = last_member_noexcept_spec;
  if (owner) defining_class_stack[nb_defining_class_stack++] = owner;
  function = make_func_type_from_saved_params(&result, parameters);
  nb_defining_class_stack = saved_class_depth;
  function.t |= cv;
  apply_function_noexcept(&function, exception_spec);
  for (parameter = function.ref->next; parameter; parameter = parameter->next) {
    if (count == 16) cprime_error("too many specialization parameters");
    actual[count] = parameter->type;
    /* Deduction takes the referred-to expression type. Preserve lvalue
       category for reference patterns without adding a pointer level. */
    if (is_reference_type(&actual[count])) {
      int category = actual[count].t & (VT_REFERENCE | VT_RVALUE_REFERENCE);
      actual[count] = *pointed_type(&actual[count]);
      actual[count].t |= category;
    }
    ++count;
  }
  if (owner) {
    TemplateMemberDef *member;
    TemplateInstOwner *record = find_template_inst_owner_entry(owner);
    int lookup = record ? record->owner->name_tok : owner;
    int class_argument = first_template_arg_tok_from_inst_type(&(CType){VT_STRUCT, struct_find(owner)});
    int old_count = nb_template_member_call_arg_types;
    int old_explicit = explicit_member_template_arg_tok;
    CType old_actual[16];
    memcpy(old_actual, template_member_call_arg_types, sizeof(old_actual));
    nb_template_member_call_arg_types = count;
    memcpy(template_member_call_arg_types, actual, count * sizeof(CType));
    ++materializing_template_interface;
    for (member = template_member_candidates(lookup, name); member; member = member->bucket_next) {
      int key, candidate;
      const CppMemberDeclInfo *info;
      if (!template_member_is_call_specialized(member)
          || template_member_def_method_tok(member) != name) continue;
      explicit_member_template_arg_tok = 0;
      key = explicit_args.nb ? intern_explicit_member_arguments(&explicit_args)
                            : deduce_member_template_argument_key(member, owner, actual, count);
      if (key <= 0) continue;
      explicit_member_template_arg_tok = explicit_args.nb ? key : 0;
      last_instantiated_member_func_tok = 0;
      instantiate_template_member_if_needed(member, class_argument, owner, key);
      candidate = last_instantiated_member_func_tok;
      info = candidate ? lookup_cpp_member_decl(candidate) : NULL;
      if (!info || info->cv_qualifiers != cv
          || !same_func_param_signature((CType *)&info->function_type, &function)
          || !compare_types(&info->function_type.ref->type, &result, 1)) continue;
      if (function_tok && function_tok != candidate) {
        int order = compare_member_function_template_order(candidate, function_tok);
        if (order < 0) continue;
        if (!order) cprime_error("ambiguous explicit member template specialization '%s'", get_tok_str(name, NULL));
      }
      function_tok = candidate;
    }
    compile_pending_template_specs_without_member_flush();
    --materializing_template_interface;
    explicit_member_template_arg_tok = old_explicit;
    nb_template_member_call_arg_types = old_count;
    memcpy(template_member_call_arg_types, old_actual, sizeof(old_actual));
    if (!function_tok && !explicit_args.nb && (instantiation || record)) {
      MemberFuncOverload *ordinary;
      for (ordinary = member_func_overloads; ordinary; ordinary = ordinary->next) {
        const CppMemberDeclInfo *info;
        if (ordinary->struct_tok != owner || ordinary->method_tok != name) continue;
        info = lookup_cpp_member_decl(ordinary->mangled_tok);
        if (!info || info->cv_qualifiers != cv
            || !same_func_param_signature((CType *)&info->function_type, &function)
            || !compare_types(&info->function_type.ref->type, &result, 1)) continue;
        if (function_tok && function_tok != ordinary->mangled_tok)
          cprime_error("ambiguous explicit member instantiation");
        function_tok = ordinary->mangled_tok;
      }
      if (function_tok && instantiation == 1) {
        CType receiver = { VT_STRUCT, struct_find(owner) };
        instantiate_template_member_body_for_func_tok(&receiver, name, function_tok);
      }
    }
  } else {
    TemplateDef *candidate;
    for (candidate = template_def_candidates(qualified); candidate;
         candidate = candidate->lookup_bucket_next) {
      TemplateArgList arguments;
      int candidate_tok;
      if (candidate->is_class || candidate->is_alias
          || template_def_lookup_tok(candidate) != qualified
          || candidate->func_max_args != count) continue;
      if (!infer_template_args_from_call_with_explicit(candidate, actual, count,
                          &arguments, explicit_args.nb ? &explicit_args : NULL)) continue;
      candidate_tok = instantiate_template_if_needed(candidate, &arguments);
      compile_pending_template_specs_without_member_flush();
      symbol = global_symbol_find(candidate_tok);
      if (!symbol || !same_func_param_signature(&symbol->type, &function)
          || !compare_types(&symbol->type.ref->type, &result, 1)) continue;
      if (function_tok && function_tok != candidate_tok) {
        int order = compare_free_function_template_order(candidate, selected_free);
        if (order < 0) continue;
        if (!order) cprime_error("ambiguous explicit function template specialization '%s'", get_tok_str(name, NULL));
      }
      function_tok = candidate_tok; selected_free = candidate;
    }
  }
  if (!function_tok)
    cprime_error("no matching function template for specialization '%s'", get_tok_str(name, NULL));
  symbol = global_symbol_find(function_tok);
  if (instantiation) {
    skip(';');
    if (!symbol) cprime_error("explicit instantiation has no function symbol");
    /* Register emission demand even when this translation unit never calls
       the function. The normal deferred drain supplies the primary's body. */
    if (instantiation == 1) put_extern_sym(symbol, NULL, 0, 0);
    tok_str_free(parameters);
    tok_str_free(definition);
    return;
  }
  for (i = 0; i < nb_explicit_function_specializations; ++i)
    if (explicit_function_specializations[i] == function_tok) break;
  if (i == nb_explicit_function_specializations) {
    if (symbol && symbol->c)
      cprime_error("specialization after instantiation '%s'", get_tok_str(name, NULL));
    explicit_function_specializations = cprime_realloc(explicit_function_specializations,
        (nb_explicit_function_specializations + 1) * sizeof(int));
    explicit_function_specializations[nb_explicit_function_specializations++] = function_tok;
  }
  /* The selected primary contributes its interface and native template
     identity, while this declaration supplies the specialization's body. */
  if (owner) {
    int m, j;
    for (m = 0; m < nb_pending_member_funcs; ++m) {
      PendingMemberFunc *pending = pending_member_funcs[m];
      if (pending && pending->func_tok == function_tok) {
        unindex_pending_auto_member(pending);
        tok_str_free(pending->str);
        cprime_free(pending);
        pending_member_funcs[m] = NULL;
      }
    }
    for (m = 0; m < nb_template_member_defs; ++m) {
      TemplateMemberDef *member = template_member_defs[m];
      for (j = 0; j < member->nb_inst; ++j)
        if (member->inst_func_toks[j] == function_tok) member->inst_states[j] = 2;
    }
  } else {
    for (i = 0; i < selected_free->nb_inst; ++i)
      if (selected_free->inst_name_toks[i] == function_tok)
        selected_free->inst_states[i] = 2;
  }
  if (symbol) {
    symbol->a.weak = is_inline;
    if (is_inline) symbol->type.t |= VT_INLINE;
    else symbol->type.t &= ~VT_INLINE;
    update_storage(symbol);
  }
  if (owner) tok_str_add(definition, name);
  else {
    add_ctype_tokens(definition, &result);
    tok_str_add(definition, function_tok);
  }
  tok_str_add(definition, '(');
  for (i = 0; i < parameters->len && parameters->str[i] != TOK_EOF; ++i)
    tok_str_add_record(definition, parameters->str, parameters->len, &i);
  tok_str_add(definition, ')');
  if (cv & VT_CONSTANT) tok_str_add(definition, TOK_CONST1);
  if (cv & VT_VOLATILE) tok_str_add(definition, TOK_VOLATILE1);
  append_function_exception_spec(definition, &function.ref->f);
  if (tok == '{') {
    TokenString *body = NULL;
    skip_or_save_block(&body);
    tok_str_append(definition, body);
    tok_str_free(body);
  } else {
    skip(';'); tok_str_add(definition, ';');
  }
  tok_str_add(definition, TOK_EOF);
  saved_tok = tok; saved_tokc = tokc;
  enter_namespace_scope(owner ? symbol_namespace_scope_tok(owner) : selected_free->namespace_tok);
  begin_macro(definition, 1); next();
  if (owner) parse_cpp_scoped_member_def_body(&result, owner, function_tok);
  else decl(VT_CONST);
  end_macro(); tok = saved_tok; tokc = saved_tokc;
  enter_namespace_scope(saved_namespace);
  tok_str_free(parameters);
}
