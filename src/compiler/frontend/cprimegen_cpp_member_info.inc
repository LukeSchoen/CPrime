/* Source-level member declarations survive lowering to ordinary functions.
   The symbol spelling is an internal lookup key; ABI naming must use these
   semantic fields instead of decoding that spelling. */
static const CppMemberDeclInfo *lookup_cpp_member_decl(int function_tok)
{
  function_tok &= ~SYM_FIELD;
  return (unsigned)function_tok < cpp_member_declaration_capacity
       ? cpp_member_declarations[function_tok] : NULL;
}

static CppMemberDeclInfo *note_cpp_member_decl(int owner_tok, int source_name_tok,
    int function_tok, CType *function_type, int is_static, int is_lowered)
{
  CppMemberDeclInfo *member;
  CppMemberDeclContext *context;
  VirtualMethodInfo *virtual_method;
  Sym *owner;
  unsigned bucket;
  function_tok &= ~SYM_FIELD;
  if (!owner_tok || !source_name_tok || !function_tok) return NULL;
  bucket = (unsigned)function_tok;
  if (bucket >= cpp_member_declaration_capacity) {
    unsigned old = cpp_member_declaration_capacity;
    cpp_member_declaration_capacity = old ? old : 1024;
    while (bucket >= cpp_member_declaration_capacity) cpp_member_declaration_capacity *= 2;
    cpp_member_declarations = cprime_realloc(cpp_member_declarations,
      cpp_member_declaration_capacity * sizeof(*cpp_member_declarations));
    memset(cpp_member_declarations + old, 0,
      (cpp_member_declaration_capacity - old) * sizeof(*cpp_member_declarations));
  }
  member = (CppMemberDeclInfo *)lookup_cpp_member_decl(function_tok);
  if (!member) {
    member = cprime_mallocz(sizeof(*member));
    member->function_tok = function_tok;
    member->owner_tok = owner_tok;
    member->source_name_tok = source_name_tok;
    owner = struct_find(owner_tok);
    member->access = owner && owner->a.is_class_tag ? 2 : 0;
    cpp_member_declarations[bucket] = member;
    member->owner_next = cpp_member_owner_declarations[(unsigned)owner_tok & 1023];
    cpp_member_owner_declarations[(unsigned)owner_tok & 1023] = member;
  }
  for (context = member->access_known ? NULL : cpp_member_decl_context;
       context; context = context->previous)
    if (context->owner_tok == owner_tok) {
      member->access = context->access;
      member->access_known = 1;
      break;
    }
  member->is_static = is_static != 0;
  member->kind = source_name_tok == TOK_CONSTRUCTOR1 ? 1
                 : source_name_tok == TOK_DESTRUCTOR1 ? 2 : 0;
  if (function_type && function_type->ref) {
    function_type->ref->f.func_cpp_member = !is_static;
    member->function_type = *function_type;
    member->function_prototype = *function_type->ref;
    member->function_type.ref = &member->function_prototype;
    /* Body replay may use a lowered signature which omits source-level
       function cv flags. Keep the declaration's qualifiers authoritative. */
    member->cv_qualifiers |= function_type->t & (VT_CONSTANT | VT_VOLATILE);
    if (function_type->t & (VT_FUNC_LREF | VT_FUNC_RREF))
      member->ref_qualifier = function_type->t & VT_FUNC_RREF ? 2 : 1;
    else if (function_type->ref->f.func_ref_qualifier)
      member->ref_qualifier = function_type->ref->f.func_ref_qualifier;
    if (is_lowered && !is_static && member->function_prototype.next) {
      Sym *receiver = member->function_prototype.next;
      if ((receiver->type.t & VT_BTYPE) == VT_PTR)
        member->cv_qualifiers |= pointed_type(&receiver->type)->t
                                  & (VT_CONSTANT | VT_VOLATILE);
      member->function_prototype.next = receiver->next;
    }
    if (function_type->ref->f.func_cpp_conversion) member->kind = 3;
  }
  for (virtual_method = virtual_method_mangled[(unsigned)function_tok & 1023];
       virtual_method; virtual_method = virtual_method->mangled_next)
    if (virtual_method->class_tok == owner_tok
        && virtual_method->mangled_tok == function_tok) {
      member->is_virtual = 1;
      break;
    }
  owner = struct_find(owner_tok);
  if (member->kind == 2 && owner) {
    CType owner_type;
    owner_type.t = owner->type.t;
    owner_type.ref = owner;
    /* Inherited virtuality already applies to the first destructor
       declaration, before end-of-class vtable publication. */
    if (class_has_virtual_destructor(&owner_type))
      member->is_virtual = 1;
  }
  return member;
}

static void note_cpp_template_member_decl(TemplateMemberDef *definition,
    int owner_tok, int source_name_tok, int function_tok, CType *function_type,
    int is_static, int is_lowered)
{
  CppMemberDeclInfo *member = note_cpp_member_decl(owner_tok, source_name_tok,
      function_tok, function_type, is_static, is_lowered);
  if (member && definition && definition->member_access_known) {
    member->access = definition->member_access;
    member->access_known = 1;
  }
}

static void free_cpp_member_declarations(void)
{
  int bucket;
  while (cpp_class_friends) {
    CppClassFriend *next = cpp_class_friends->next;
    cprime_free(cpp_class_friends);
    cpp_class_friends = next;
  }
  for (bucket = 0; bucket < cpp_member_declaration_capacity; ++bucket)
    cprime_free(cpp_member_declarations[bucket]);
  cprime_free(cpp_member_declarations);
  cpp_member_declarations = NULL;
  cpp_member_declaration_capacity = 0;
  for (bucket = 0; bucket < 1024; ++bucket) {
    cpp_member_owner_declarations[bucket] = NULL;
    {
      CppRecordDeclInfo *record = cpp_record_declarations[bucket];
      while (record) {
        CppRecordDeclInfo *next = record->next;
        cprime_free(record);
        record = next;
      }
      cpp_record_declarations[bucket] = NULL;
    }
  }
  cpp_member_decl_context = NULL;
}

static int cpp_class_is_friend(int owner, int candidate)
{
  CppClassFriend *entry;
  if (!owner || !candidate) return 0;
  for (entry = cpp_class_friends; entry; entry = entry->next)
    if (entry->owner == owner && entry->candidate == candidate) return 1;
  return 0;
}

static void cpp_note_class_friend(int owner, int candidate)
{
  CppClassFriend *entry;
  if (!owner || !candidate || cpp_class_is_friend(owner, candidate)) return;
  entry = cprime_malloc(sizeof(*entry));
  entry->owner = owner;
  entry->candidate = candidate;
  entry->next = cpp_class_friends;
  cpp_class_friends = entry;
}

static void check_cpp_function_access(Sym *function)
{
  const CppMemberDeclInfo *member;
  if (!function) return;
  member = lookup_cpp_member_decl(function->v);
  if (member && member->access_known && member->access
      && active_member_class_tok != member->owner_tok
      && !cpp_class_is_friend(member->owner_tok, active_member_class_tok)
      && !(member->access == 1 && active_member_class_tok
           && class_has_base(active_member_class_tok, member->owner_tok)))
    cprime_error("member function '%s' is inaccessible",
                 get_tok_str(member->source_name_tok, NULL));
}

static const CppRecordDeclInfo *lookup_cpp_record_decl(int type_tok)
{
  CppRecordDeclInfo *record;
  type_tok &= ~SYM_STRUCT;
  for (record = cpp_record_declarations[(unsigned)type_tok & 1023];
       record; record = record->next)
    if (record->type_tok == type_tok) return record;
  return NULL;
}

static void note_cpp_record_decl(int type_tok, int source_name_tok,
                                 int owner_tok, int namespace_tok)
{
  CppRecordDeclInfo *record;
  unsigned bucket;
  type_tok &= ~SYM_STRUCT;
  if (type_tok < TOK_IDENT || source_name_tok < TOK_IDENT
      || source_name_tok >= SYM_FIRST_ANOM || lookup_cpp_record_decl(type_tok)) return;
  bucket = (unsigned)type_tok & 1023;
  record = cprime_mallocz(sizeof(*record));
  record->type_tok = type_tok;
  record->source_name_tok = source_name_tok;
  record->owner_tok = owner_tok == type_tok ? 0 : owner_tok;
  record->lexical_owner_tok = record->owner_tok;
  record->namespace_tok = namespace_tok;
  record->next = cpp_record_declarations[bucket];
  cpp_record_declarations[bucket] = record;
}

/* The first typedef naming an unnamed class/enum supplies its linkage
   name. Keep its lexical owner rather than anonymous allocation order. */
static void note_cpp_typedef_record_name(CType *type, int alias_tok)
{
  int tag, namespace_tok;
  if (!is_cpp_translation_unit() || !type->ref || (type->t & VT_ARRAY)
      || ((type->t & VT_BTYPE) != VT_STRUCT && !IS_ENUM(type->t))) return;
  tag=type->ref->v & ~SYM_STRUCT;
  if (tag < SYM_FIRST_ANOM || lookup_cpp_record_decl(tag)) return;
  namespace_tok=current_namespace_scope_tok();
  note_cpp_record_decl(tag, cpp_ms_unqualified_tok(alias_tok,namespace_tok),
      nb_defining_class_stack ? defining_class_stack[nb_defining_class_stack-1] : 0,
      namespace_tok);
}

static void note_cpp_record_template(int type_tok, TemplateDef *definition,
                                      TemplateArgList *arguments)
{
  const CppRecordDeclInfo *origin = lookup_cpp_record_decl(definition->lookup_tok);
  CppRecordDeclInfo *record;
  note_cpp_record_decl(type_tok, origin ? origin->source_name_tok : definition->name_tok,
                      origin ? origin->owner_tok : 0, definition->namespace_tok);
  record = (CppRecordDeclInfo *)lookup_cpp_record_decl(type_tok);
  if (record && !record->template_definition) {
    /* A forward replay can introduce the lowered token first. The template
       declaration supplies the source spelling and lexical owner. */
    record->source_name_tok = origin ? origin->source_name_tok : definition->name_tok;
    record->owner_tok = origin ? origin->owner_tok : 0;
    record->namespace_tok = definition->namespace_tok;
    record->template_definition = definition;
    record->template_arguments = *arguments;
  }
}

static void note_cpp_member_template_arguments(int function_tok,
    TemplateArgList *arguments, unsigned value_mask)
{
  CppMemberDeclInfo *member = (CppMemberDeclInfo *)lookup_cpp_member_decl(function_tok);
  if (!member) cprime_error("member template has no semantic declaration");
  member->is_template = 1;
  member->template_arguments = *arguments;
  member->template_value_mask = value_mask;
}

static int canonical_cpp_member_template_tok(int candidate_tok, int owner_tok,
    int source_tok, CType *function_type, TemplateArgList *arguments,
    unsigned value_mask)
{
  CppMemberDeclInfo *member;
  for (member = cpp_member_owner_declarations[(unsigned)owner_tok & 1023];
       member; member = member->owner_next) {
    int i;
    if (member->owner_tok != owner_tok || member->source_name_tok != source_tok
        || !member->is_template || member->template_arguments.nb != arguments->nb
        || member->template_value_mask != value_mask
        || member->cv_qualifiers != (function_type->t & (VT_CONSTANT | VT_VOLATILE))
        || !same_func_param_signature(&member->function_type, function_type))
      continue;
    for (i = 0; i < arguments->nb; ++i) {
      if (i < 32 && (value_mask & (1u << i))) {
        int64_t first, second;
        if (member->template_arguments.toks[i] == arguments->toks[i]) continue;
        if (!template_arg_const_int64(member->template_arguments.toks[i], &first)
            || !template_arg_const_int64(arguments->toks[i], &second)
            || first != second) break;
      } else if (!template_type_arg_toks_match(member->template_arguments.toks[i],
                                               arguments->toks[i])) break;
    }
    if (i == arguments->nb) return member->function_tok;
  }
  return candidate_tok;
}
