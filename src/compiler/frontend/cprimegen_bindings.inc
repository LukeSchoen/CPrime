typedef struct CppBindingMember {
  CType type;
  int offset;
} CppBindingMember;

typedef struct CppBindingMembers {
  CppBindingMember *items;
  int count, capacity, owner, owner_offset;
} CppBindingMembers;

/* Decomposition uses all data members of one class subobject. Empty bases
   contribute nothing; two distinct data-bearing subobjects cannot be mixed. */
static void collect_cpp_binding_members(CType *type, int offset,
                                        CppBindingMembers *members)
{
  Sym *field;
  int owner = get_struct_type_name_tok(type);
  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref || IS_UNION(type->t))
    cprime_error("structured binding requires an array or non-union class");
  for (field = type->ref->next; field; field = field->next)
  {
    int name = field->v & ~SYM_FIELD;
    ClassBaseInfo *base = class_direct_base_for_field(owner, field, NULL);
    if (base) {
      CType base_type = field->type;
      int before = members->count;
      base_type.t |= type->t & (VT_CONSTANT | VT_VOLATILE);
      collect_cpp_binding_members(&base_type, offset + field->c, members);
      if (members->count != before && base->access)
        cprime_error("structured binding base class is not public");
      continue;
    }
    if (!cpp_field_is_data_member(field) || (field->type.t & VT_TYPEDEF))
      continue;
    if (name < TOK_UIDENT || name >= SYM_FIRST_ANOM) {
      if (field->type.t & VT_BITFIELD) continue; /* unnamed bit-field */
      cprime_error("structured binding cannot decompose an anonymous member");
    }
    if (field->a.cpp_field_access
        && active_member_class_tok != owner
        && !(field->a.cpp_field_access == 1 && active_member_class_tok
             && class_has_base(active_member_class_tok, owner)))
      cprime_error("structured binding member '%s' is inaccessible", get_tok_str(name, NULL));
    if (members->owner && (members->owner != owner || members->owner_offset != offset))
      cprime_error("structured binding members belong to different class subobjects");
    members->owner = owner;
    members->owner_offset = offset;
    if (members->count == members->capacity) {
      members->capacity = members->capacity ? members->capacity * 2 : 8;
      members->items = cprime_realloc(members->items,
                                      members->capacity * sizeof(*members->items));
    }
    members->items[members->count].type = field->type;
    if (!is_reference_type(&field->type)) {
      int cv = type->t & (VT_CONSTANT | VT_VOLATILE);
      if (field->a.cpp_mutable_field) cv &= ~VT_CONSTANT;
      members->items[members->count].type.t |= cv;
    }
    members->items[members->count++].offset = offset + field->c;
  }
}

static int cpp_binding_tuple_protocol(CType *type)
{
  int names[2] = {tok_alloc_const("std"), tok_alloc_const("tuple_size")};
  TokenString *probe;
  CType tuple_size;
  int valid;
  if (!find_class_template_def(find_namespace_tok_from_parts(names, 2)))
    return 0;
  probe = tok_str_alloc();
  tok_str_add(probe, names[0]); tok_str_add(probe, ':'); tok_str_add(probe, ':');
  tok_str_add(probe, names[1]); tok_str_add(probe, TOK_LT);
  tok_str_add(probe, template_exact_type_tok_from_ctype(type));
  tok_str_add(probe, TOK_GT); tok_str_add(probe, TOK_EOF);
  valid = template_probe_type_ex(probe, 6, current_namespace_scope_tok(), 0, 0, &tuple_size);
  tok_str_free(probe);
  return valid && (tuple_size.t & VT_BTYPE) == VT_STRUCT
         && tuple_size.ref && tuple_size.ref->c >= 0;
}

static TokenString *cpp_binding_tuple_trait(CType *type, const char *trait, int index)
{
  TokenString *tokens = tok_str_alloc();
  tok_str_add(tokens, tok_alloc_const("std"));
  tok_str_add(tokens, ':'); tok_str_add(tokens, ':');
  tok_str_add(tokens, tok_alloc_const(trait));
  tok_str_add(tokens, TOK_LT);
  if (index >= 0) { tok_str_add_cint(tokens, index); tok_str_add(tokens, ','); }
  tok_str_add(tokens, template_exact_ctype_typedef_tok(type));
  tok_str_add(tokens, TOK_GT);
  tok_str_add(tokens, ':'); tok_str_add(tokens, ':');
  return tokens;
}

static void cpp_binding_tuple_members(CType *type, int count, CppBindingMembers *members)
{
  TokenString *tokens = cpp_binding_tuple_trait(type, "tuple_size", -1);
  TokenString *parent = macro_stack;
  const int *parent_ptr = macro_ptr;
  int saved = tok, i;
  int64_t size;
  CValue value = tokc;
  tok_str_add(tokens, tok_alloc_const("value")); tok_str_add(tokens, TOK_EOF);
  begin_macro(tokens, 1); next();
  size = expr_const64();
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr; tok = saved; tokc = value;
  if (size != count)
    cprime_error("structured binding has %d names but tuple has %lld elements", count, size);
  members->items = cprime_mallocz(count * sizeof(*members->items));
  members->count = count;
  for (i = 0; i < count; ++i) {
    int valid;
    tokens = cpp_binding_tuple_trait(type, "tuple_element", i);
    tok_str_add(tokens, tok_alloc_const("type")); tok_str_add(tokens, TOK_EOF);
    valid = template_probe_type_ex(tokens, 6, current_namespace_scope_tok(), 0, 0,
                                   &members->items[i].type);
    tok_str_free(tokens);
    if (!valid) cprime_error("structured binding tuple_element has no type");
  }
}

static void cpp_binding_receiver(TokenString *tokens, CType *type, int hidden, int rvalue)
{
  if (rvalue) {
    CType reference = *type;
    mk_reference(&reference); reference.t |= VT_RVALUE_REFERENCE;
    tok_str_add(tokens, CPP_TOK_STATIC_CAST); tok_str_add(tokens, TOK_LT);
    tok_str_add(tokens, template_exact_ctype_typedef_tok(&reference));
    tok_str_add(tokens, TOK_GT); tok_str_add(tokens, '(');
  }
  tok_str_add(tokens, hidden);
  if (rvalue) tok_str_add(tokens, ')');
}

static int cpp_binding_member_get(int owner)
{
  int get = tok_alloc_const("get");
  int found = 0;
  TemplateMemberDef *member;
  TemplateInstOwner *instance;
  ClassBaseInfo *base;
  for (member = template_member_candidates(owner, get); member; member = member->bucket_next)
    if (member->class_tok == owner && template_member_def_method_tok(member) == get) {
      int at;
      found = 1;
      if (member->nb_stripped_member_params) {
        if (member->stripped_member_value_mask & 1) return 1;
        continue;
      }
      for (at = 0; at < member->def_str->len; ++at)
        if (is_template_keyword_tok(member->def_str->str[at])) {
          at = tok_str_next_non_linenum_index(member->def_str->str, member->def_str->len, at + 1);
          at = tok_str_next_non_linenum_index(member->def_str->str, member->def_str->len, at + 1);
          if (at < member->def_str->len && member->def_str->str[at] != TOK_CLASS
            && member->def_str->str[at] != CPP_TOK_TYPENAME
            && !is_template_keyword_tok(member->def_str->str[at])) return 1;
          break;
        }
    }
  if (found) return 0;
  instance = find_template_inst_owner_entry(owner);
  if (instance && instance->owner->name_tok != owner
      && cpp_binding_member_get(instance->owner->name_tok)) return 1;
  for (base = class_base_candidates(owner); base; base = base->bucket_next)
    if (base->class_tok == owner && cpp_binding_member_get(base->base_tok)) return 1;
  return 0;
}

/* Decomposition's free get lookup is ADL-only. Resolve the explicit index
   against associated declarations, then replay the selected concrete call. */
static int cpp_binding_adl_get(CType *type, int rvalue, int index)
{
  typedef struct BindingGetCandidate {
    TemplateDef *definition;
    TemplateArgList bindings;
    int rank;
  } BindingGetCandidate;
  CppOperatorLookup lookup = {0};
  BindingGetCandidate *candidates = NULL;
  TemplateArgList explicit_arguments = {0};
  CType argument = *type;
  Sym **saved_classes = template_adl_classes;
  int saved_class_count = template_adl_class_count;
  int name, count = 0, i, j, selected = -1, result;
  lookup.method_tok = tok_alloc_const("get");
  cpp_operator_associated_type(&lookup, &argument, 0);
  if (rvalue) argument.t |= VT_RVALUE_REFERENCE;
  explicit_arguments.nb = 1;
  explicit_arguments.toks[0] = template_integer_expression_arg_tok(index);
  template_adl_classes = lookup.visited;
  template_adl_class_count = lookup.visited_count;
  for (name = 0; name < lookup.name_count; ++name) {
    TemplateDef *definition;
    for (definition = template_def_candidates(lookup.names[name]); definition;
         definition = definition->lookup_bucket_next) {
      TemplateArgList bindings;
      int rank;
      if (definition->is_class || definition->is_alias || definition->is_guide
          || !template_function_matches_namespace(definition, lookup.names[name])
          || definition->func_min_args > 1
          || (!definition->func_is_variadic && definition->func_max_args < 1)
          || !definition->nb_type_params || !(definition->value_param_mask & 1)
          || template_definition_has_unbound_parameter(definition, &explicit_arguments)) continue;
      for (i = 0; i < count; ++i) if (candidates[i].definition == definition) break;
      if (i != count) continue;
      if (!infer_template_args_from_call_with_explicit(definition, &argument, 1,
                                                       &bindings, &explicit_arguments)
          || !template_return_substitution_matches_bound(definition, &bindings)) continue;
      rank = template_call_signature_rank_bound(definition, &argument, 1, &bindings, 0);
      if (rank == -1) continue;
      if (rank == -2) rank = 1;
      candidates = cprime_realloc(candidates, (count + 1) * sizeof(*candidates));
      candidates[count].definition = definition;
      candidates[count].bindings = bindings;
      candidates[count++].rank = rank;
    }
  }
  for (i = 0; i < count; ++i) {
    for (j = 0; j < count; ++j) if (i != j) {
      int preference = candidates[j].rank - candidates[i].rank;
      if (!preference)
        preference = compare_reference_binding_preference(candidates[i].definition,
          &candidates[i].bindings, candidates[j].definition, &candidates[j].bindings, &argument, 1);
      if (!preference)
        preference = compare_free_function_template_order(candidates[i].definition,
                                                           candidates[j].definition);
      if (preference <= 0) break;
    }
    if (j == count) { selected = i; break; }
  }
  template_adl_classes = saved_classes;
  template_adl_class_count = saved_class_count;
  cprime_free(lookup.names); cprime_free(lookup.visited);
  if (selected < 0) {
    cprime_free(candidates);
    cprime_error(count ? "ambiguous structured binding get" : "no associated structured binding get");
  }
  result = instantiate_template_if_needed(candidates[selected].definition,
                                           &candidates[selected].bindings);
  compile_pending_template_specs_without_member_flush();
  cprime_free(candidates);
  return result;
}
