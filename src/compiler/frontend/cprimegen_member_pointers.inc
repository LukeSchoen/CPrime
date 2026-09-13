/* Member pointers retain their declaring type independently of their storage.
   The two words hold a target/offset and a signed receiver adjustment.  The
   low adjustment bit identifies a virtual target; function addresses need
   not be aligned or tagged. */
static CppMemberPointerType *find_cpp_member_pointer_type(CType *type)
{
  CppMemberPointerType *entry;
  if ((type->t & VT_BTYPE) != VT_STRUCT)
    return NULL;
  for (entry = cpp_member_pointer_types; entry; entry = entry->next)
    if (entry->symbol == type->ref)
      return entry;
  return NULL;
}

static void mk_cpp_member_pointer(CType *type, int owner_tok)
{
  CppMemberPointerType *entry;
  CString name;
  Sym *saved_local, *symbol, *field;
  CType storage_type, word_type;
  int type_tok;
  for (entry = cpp_member_pointer_types; entry; entry = entry->next)
    if (!entry->provisional && entry->owner_tok == owner_tok
        && is_compatible_types(&entry->member_type, type))
    {
      type->t = VT_STRUCT;
      type->ref = entry->symbol;
      return;
    }
  cstr_new(&name);
  cstr_cat(&name, "__cpc_member_pointer_", -1);
  cstr_cat(&name, get_tok_str(owner_tok, NULL), -1);
  if (type->t & VT_CONSTANT) cstr_cat(&name, "_const", -1);
  if (!append_type_mangle_string(&name, type))
    cprime_error("unsupported pointer-to-member type");
  cstr_ccat(&name, 0);
  type_tok = tok_alloc_const(name.data);
  cstr_free(&name);
  entry = cprime_mallocz(sizeof(*entry));
  entry->owner_tok = owner_tok;
  entry->member_type = *type;
  saved_local = local_stack;
  local_stack = NULL;
  symbol = global_symbol_find(type_tok);
  if (!symbol) symbol = sym_find(type_tok);
  if (!symbol) symbol = struct_find(type_tok);
  if (!symbol || (symbol->type.t & VT_BTYPE) != VT_STRUCT)
  {
    storage_type.t = VT_STRUCT;
    storage_type.ref = NULL;
    symbol = sym_push(type_tok | SYM_STRUCT, &storage_type, PTR_SIZE,
                      2 * PTR_SIZE);
    symbol->sym_scope = 0;
    symbol->type.ref = symbol;
    word_type.t = VT_LLONG;
    word_type.ref = NULL;
    symbol->next = field = sym_push(tok_alloc_const("__target") | SYM_FIELD,
                                    &word_type, 0, 0);
    field->next = sym_push(tok_alloc_const("__adjustment") | SYM_FIELD,
                           &word_type, 0, PTR_SIZE);
  }
  local_stack = saved_local;
  entry->symbol = symbol;
  entry->next = cpp_member_pointer_types;
  cpp_member_pointer_types = entry;
  type->t = VT_STRUCT;
  type->ref = symbol;
}

static CType *make_provisional_cpp_member_pointer(CType *type, int owner_tok)
{
  CppMemberPointerType *entry = cprime_mallocz(sizeof(*entry));
  Sym *saved_local = local_stack;
  CType storage_type;
  char name[64];
  entry->member_type = *type;
  entry->owner_tok = owner_tok;
  entry->provisional = 1;
  storage_type.t = VT_STRUCT;
  storage_type.ref = NULL;
  snprintf(name, sizeof(name), "__cpc_member_declarator_%d", anon_sym++);
  local_stack = NULL;
  entry->symbol = sym_push(tok_alloc_const(name) | SYM_STRUCT,
                           &storage_type, PTR_SIZE, 2 * PTR_SIZE);
  entry->symbol->sym_scope = 0;
  entry->symbol->type.ref = entry->symbol;
  local_stack = saved_local;
  entry->next = cpp_member_pointer_types;
  cpp_member_pointer_types = entry;
  type->t = VT_STRUCT;
  type->ref = entry->symbol;
  return &entry->member_type;
}

static void canonicalize_cpp_member_pointer_declarator(CType *type)
{
  CppMemberPointerType *entry = find_cpp_member_pointer_type(type);
  if (entry && entry->provisional)
  {
    int qualifiers = type->t & (VT_STORAGE | VT_CONSTANT | VT_VOLATILE);
    *type = entry->member_type;
    mk_cpp_member_pointer(type, entry->owner_tok);
    type->t |= qualifiers;
  }
  else if ((type->t & VT_BTYPE) == VT_PTR)
    canonicalize_cpp_member_pointer_declarator(pointed_type(type));
  else if ((type->t & VT_BTYPE) == VT_FUNC)
  {
    Sym *parameter;
    canonicalize_cpp_member_pointer_declarator(&type->ref->type);
    for (parameter = type->ref->next; parameter; parameter = parameter->next)
      canonicalize_cpp_member_pointer_declarator(&parameter->type);
  }
}

static int cpp_member_pointer_declarator_ahead(void)
{
  TokenString *replay;
  int angle = 0, colons = 0, result = 0, qualified = 1, names = 0;
  if (tok < TOK_UIDENT) return 0;
  replay = tok_str_alloc();
  while (tok != TOK_EOF) {
    /* `(C::*name)` is a declarator group only while the leading identifiers
       form a qualified owner.  A parameter list can also start with a
       member-pointer type (`(T C::*)`), where two adjacent identifiers mean
       the group declares a function parameter instead. */
    if (!angle && tok == '*') { result = colons == 2 && qualified; break; }
    if (!angle && tok < TOK_UIDENT && tok != ':' && tok != TOK_LT) break;
    if (!angle && tok >= TOK_UIDENT) {
      if (names && colons != 2) qualified = 0;
      ++names;
    }
    if (tok == TOK_LT) ++angle;
    else if (tok == TOK_GT) --angle;
    else if (tok == TOK_SAR) angle -= 2;
    colons = tok == ':' ? colons + 1 : 0;
    tok_str_add_tok(replay);
    next();
  }
  restore_cpp_lifecycle_probe(replay);
  return result;
}

static int try_parse_cpp_member_pointer_owner(void)
{
  TokenString *replay, *owner;
  CType owner_type;
  int angle = 0, owner_tok = 0;
  int first = find_current_namespace_tok(tok);
  Sym *alias = sym_find(first);
  if (!alias) alias = global_symbol_find(first);
  if (!struct_find(first) && !find_class_template_def(first)
      && !is_namespace_tok(tok)
      && !(alias && (alias->type.t & VT_TYPEDEF)
           && (alias->type.t & VT_BTYPE) == VT_STRUCT))
  {
    /* A nested class of the class enclosing the current member declaration
       is registered only under its qualified token (Outer_Inner), so the
       unqualified spelling must still name a member-pointer owner here. */
    int class_tok = cpp_member_decl_context
                      ? cpp_member_decl_context->owner_tok
                      : active_member_class_tok;
    int scoped = class_tok ? find_class_scope_type_tok(class_tok, tok) : tok;
    if (scoped == tok)
      return 0;
    first = scoped;
  }
  replay = tok_str_alloc();
  owner = tok_str_alloc();
  for (;;)
  {
    if (tok < TOK_UIDENT)
      break;
    tok_str_add_tok(replay);
    if (owner->len == 0) tok_str_add(owner, first);
    else tok_str_add_tok(owner);
    next();
    if (tok == '<' || tok == TOK_LT)
    {
      do {
        if (tok == '<' || tok == TOK_LT) ++angle;
        else if (tok == '>' || tok == TOK_GT) --angle;
        else if (tok == TOK_SAR) angle -= 2;
        tok_str_add_tok(replay);
        tok_str_add_tok(owner);
        next();
      } while (angle > 0 && tok != TOK_EOF);
    }
    if (tok != ':') break;
    tok_str_add_tok(replay);
    next();
    if (tok != ':') break;
    tok_str_add_tok(replay);
    next();
    if (tok == '*')
    {
      tok_str_add(owner, TOK_EOF);
      if (make_type_from_saved_type_tokens(&owner_type, owner)
          && (owner_type.t & VT_BTYPE) == VT_STRUCT)
        owner_tok = get_struct_type_name_tok(&owner_type);
      /* Saved-type parsing consumes its token string. */
      owner = NULL;
      if (!owner_tok)
        cprime_error("pointer-to-member declarator requires a class type");
      next();
      break;
    }
    tok_str_add(owner, ':');
    tok_str_add(owner, ':');
  }
  if (owner) tok_str_free(owner);
  if (owner_tok) tok_str_free(replay);
  else restore_cpp_lifecycle_probe(replay);
  return owner_tok;
}

static int member_func_address_signature_matches(CppMemberPointerType *target,
                                                 Sym *selected)
{
  CType signature;
  Sym prototype;
  if (!target || (target->member_type.t & VT_BTYPE) != VT_FUNC
      || !target->member_type.ref || !selected->type.ref)
    return 1;
  signature = selected->type;
  prototype = *signature.ref;
  if (prototype.next)
  {
    signature.t |= pointed_type(&prototype.next->type)->t
                    & (VT_CONSTANT | VT_VOLATILE);
    prototype.next = prototype.next->next;
  }
  signature.ref = &prototype;
  if (target->member_type.ref->f.func_noexcept && !prototype.f.func_noexcept)
    return 0;
  prototype.f.func_noexcept = target->member_type.ref->f.func_noexcept;
  return is_compatible_types(&signature, &target->member_type);
}

static Sym *find_member_func_address_candidate(int class_tok, int member_tok,
                                               CppMemberPointerType *target,
                                               int *owner_tok, int *ambiguous)
{
  MemberFuncOverload *candidate;
  ClassBaseInfo *base;
  Sym *found = NULL;
  int class_declares_name = 0;
  if (ambiguous) *ambiguous = 0;
  for (candidate = member_func_candidates(class_tok, member_tok);
       candidate; candidate = candidate->bucket_next)
  {
    Sym *selected;
    if (candidate->struct_tok != class_tok || candidate->method_tok != member_tok)
      continue;
    /* A member declared in this class hides the same name imported from a
       base ([class.member.lookup]); `&Derived::f` must not consider a base
       overload once the derived class declares `f` itself. */
    class_declares_name = 1;
    selected = global_symbol_find(candidate->mangled_tok);
    if (!selected) selected = sym_find(candidate->mangled_tok);
    if (!selected || !member_func_address_signature_matches(target, selected))
      continue;
    if (found && found != selected)
    {
      if (!target)
      {
        if (ambiguous) *ambiguous = 1;
        return NULL;
      }
      cprime_error("overloaded member address requires a target member-pointer type");
    }
    found = selected;
    if (owner_tok) *owner_tok = class_tok;
  }
  if (class_declares_name)
    return found;
  for (base = class_base_candidates(class_tok); base; base = base->bucket_next)
  {
    Sym *base_found;
    int base_owner = 0;
    if (base->class_tok != class_tok)
      continue;
    base_found = find_member_func_address_candidate(base->base_tok, member_tok,
                                                    target, &base_owner, ambiguous);
    if (!base_found)
      continue;
    if (found && found != base_found)
    {
      if (!target)
      {
        if (ambiguous) *ambiguous = 1;
        return NULL;
      }
      cprime_error("overloaded member address requires a target member-pointer type");
    }
    found = base_found;
    if (owner_tok) *owner_tok = base_owner;
  }
  return found;
}

static int cpp_deferred_member_address_matches(CppMemberPointerType *target,
                                               CppMemberPointerType *source)
{
  int owner_tok = 0;
  if (!target || !source || !source->deferred)
    return 0;
  return find_member_func_address_candidate(source->owner_tok,
                                            source->deferred_member_tok,
                                            target, &owner_tok, NULL) != NULL;
}

static int make_deferred_cpp_member_pointer(int owner_tok, int member_tok)
{
  CType type = {0};
  CppMemberPointerType *entry;
  CType *member_slot = make_provisional_cpp_member_pointer(&type, owner_tok);
  entry = find_cpp_member_pointer_type(&type);
  if (!entry)
    return 0;
  entry->deferred = 1;
  entry->deferred_member_tok = member_tok;
  if (member_slot)
    member_slot->t = VT_FUNC;
  vset(&type, VT_CONST, 0);
  vtop->sym = sym_push2(&global_stack, anon_sym++, type.t, 0);
  vtop->sym->type = type;
  vtop->sym->cpp_member_address_tok = member_tok;
  forming_cpp_member_address = 0;
  return 1;
}

static int resolve_deferred_cpp_member_address(CType *target_type)
{
  CppMemberPointerType *target = find_cpp_member_pointer_type(target_type);
  CppMemberPointerType *source = find_cpp_member_pointer_type(&vtop->type);
  CType *saved_target = cpp_member_address_target;
  SValue marker;
  int resolved;
  if (!target || !source || !source->deferred)
    return 0;
  marker = *vtop;
  vpop();
  cpp_member_address_target = target_type;
  resolved = try_form_cpp_member_pointer(source->owner_tok,
                                        source->deferred_member_tok);
  cpp_member_address_target = saved_target;
  if (!resolved)
    vpushv(&marker);
  return resolved;
}

static int try_form_cpp_member_pointer(int class_tok, int member_tok)
{
  CType owner_type, member_type;
  Sym *field, *function = NULL, *saved_local;
  VirtualMethodInfo *vm = NULL;
  int offset = 0, function_owner_tok = class_tok, function_owner_offset = 0;
  int member_owner_tok = class_tok, function_owner_offset_known = 0;
  size_t location;
  uint64_t words[2] = {0, 0};
  if (!make_class_type_from_tok(&owner_type, class_tok))
    return 0;
  class_tok = get_struct_type_name_tok(&owner_type);
  if (!class_tok)
    return 0;
  function_owner_tok = class_tok;
  /* A static overload of the name is not a member-pointer candidate.  Keep the
     plain function path when the contextual type cannot be a pointer to member,
     but a `&C::f` that is initializing one still resolves against the instance
     overloads, which a static overload must not hide. */
  if (class_has_static_member_func(class_tok, member_tok)
      && !(cpp_member_address_target
           && find_cpp_member_pointer_type(cpp_member_address_target)))
    return 0;
  field = find_field_try_with_owner(&owner_type, member_tok, &offset,
                                    &member_owner_tok);
  if (field && is_reference_type(&field->type))
  {
    /* A reference member has no address of its own: `&(C::m)` must reach
       the referent through ordinary member access instead of forming a
       pointer to member of reference type (CWG 4379). */
    return 0;
  }
  if (field && (field->type.t & VT_BTYPE) != VT_FUNC)
  {
    member_type = field->type;
    /* `&Derived::member` names the class that declares the member, so the
       stored offset is relative to that class; the derived-to-base subobject
       offset is re-applied by the ordinary member-pointer conversion. */
    if (member_owner_tok != class_tok)
    {
      int owner_offset = 0;
      if (class_nonvirtual_subobject_offset(class_tok, member_owner_tok,
                                            &owner_offset))
        offset -= owner_offset;
      else
        member_owner_tok = class_tok;
    }
    words[0] = (uint64_t)offset + 1;
  }
  else
  {
    Sym *prototype;
    MemberFuncOverload *candidate;
    int ambiguous = 0;
    member_owner_tok = class_tok;
    if (class_or_inst_has_member_template_name(class_tok, member_tok)
        && (cpp_member_address_target || tok == TOK_LT || tok == '<')) {
      CppMemberPointerType *target = cpp_member_address_target
          ? find_cpp_member_pointer_type(cpp_member_address_target) : NULL;
      int saved_explicit = explicit_member_template_arg_tok, count = 0;
      int parsed_explicit = 0;
      CType arguments[CPC_MAX_CALL_ARGUMENTS];
      if (tok == TOK_LT || tok == '<') {
        TemplateArgList explicit_arguments;
        parse_template_type_args(&explicit_arguments);
        explicit_member_template_arg_tok = intern_explicit_member_arguments(&explicit_arguments);
        parsed_explicit = 1;
      }
      if (target && (target->member_type.t & VT_BTYPE) == VT_FUNC) {
        Sym *parameter;
        for (parameter = target->member_type.ref->next; parameter; parameter = parameter->next) {
          if (count == CPC_MAX_CALL_ARGUMENTS) cprime_error("too many member-pointer parameters");
          arguments[count] = parameter->type;
          if (is_reference_type(&arguments[count])) {
            int category = arguments[count].t & (VT_REFERENCE | VT_RVALUE_REFERENCE);
            arguments[count] = *pointed_type(&arguments[count]);
            arguments[count].t |= category;
          }
          ++count;
        }
        instantiate_template_member_for_call(&owner_type, member_tok, arguments, count);
      }
      else if (parsed_explicit)
        /* A non-type template argument such as `&A::good<int>` names a member
           specialization without any contextual member-pointer type to deduce
           from, so the explicit argument list is the whole specification. */
        instantiate_template_member_for_call(&owner_type, member_tok, NULL, -1);
      explicit_member_template_arg_tok = saved_explicit;
    }
    if (member_tok == CPP_TOK_ASSIGN)
      materialize_implicit_copy_assignment_address(&owner_type);
    function = find_member_func_address_candidate(class_tok, member_tok,
        cpp_member_address_target
          ? find_cpp_member_pointer_type(cpp_member_address_target) : NULL,
        &function_owner_tok, &ambiguous);
    if (ambiguous && !cpp_member_address_target)
      return make_deferred_cpp_member_pointer(class_tok, member_tok);
    if (!function)
      function = resolve_member_func(&owner_type, member_tok);
    if (!function || (function->type.t & VT_BTYPE) != VT_FUNC)
      return 0;
    if (function_owner_tok == class_tok)
      function_owner_offset_known = 1;
    else if (class_nonvirtual_subobject_offset(class_tok, function_owner_tok,
                                               &function_owner_offset))
      function_owner_offset_known = 1;
    else
    {
      /* A virtual base has no constant offset; the address keeps the
         qualifying class as its owner, as before. */
      if (!class_subobject_offset(class_tok, function_owner_tok,
                                  &function_owner_offset))
        function_owner_offset = 0;
    }
    member_type = function->type;
    if (member_type.ref->next
        && (member_type.ref->next->type.t & VT_BTYPE) == VT_PTR)
      member_type.t |= pointed_type(&member_type.ref->next->type)->t
                      & (VT_CONSTANT | VT_VOLATILE);
    saved_local = local_stack;
    local_stack = NULL;
    prototype = sym_push(SYM_FIELD, &member_type.ref->type, 0, 0);
    prototype->f = member_type.ref->f;
    prototype->next = member_type.ref->next ? member_type.ref->next->next : NULL;
    local_stack = saved_local;
    member_type.ref = prototype;
    vm = find_virtual_method_for_symbol(class_tok, function->v);
    if (vm)
    {
      int vptr_offset = virtual_vptr_offset(class_tok, vm->root_tok);
      int root_offset = 0;
      if (vptr_offset < 0)
        cprime_error("pointer-to-member virtual owner is not a base subobject");
      if (class_tok != vm->root_tok
          && !class_subobject_offset(class_tok, vm->root_tok, &root_offset))
        cprime_error("pointer-to-member virtual root is not a base subobject");
      words[0] = ((uint64_t)(vptr_offset - root_offset) << 32) | (unsigned)vm->slot;
      words[1] = (uint64_t)root_offset * 2 + 1;
    }
    else if (function_owner_offset_known)
    {
      /* The pointer names the declaring class, so its adjustment is relative
         to that class rather than to the qualified class. */
      member_owner_tok = function_owner_tok;
      words[1] = 0;
    }
    else
      words[1] = (uint64_t)function_owner_offset * 2;
  }
  if (cpp_member_address_target)
  {
    /* The address is about to be converted to the contextual member-pointer
       type.  A global constant initializer cannot run the adjustment code
       that conversion would emit, so fold the subobject offset into the
       stored words here and let the conversion become an identity. */
    CppMemberPointerType *target =
      find_cpp_member_pointer_type(cpp_member_address_target);
    int adjust;
    if (target
        && is_compatible_unqualified_types(&target->member_type, &member_type)
        && !((member_type.t & ~target->member_type.t)
             & (VT_CONSTANT | VT_VOLATILE))
        && class_subobject_offset(target->owner_tok, member_owner_tok, &adjust))
    {
      /* The low bit describes virtual dispatch; the remaining bits are still
         the receiver adjustment and can be folded for a constant initializer.
         A base virtual member pointer converted to a derived owner otherwise
         requires runtime code, which a global initializer never emits. */
      words[1] = (uint64_t)((long long)words[1] + (long long)adjust * 2);
      member_owner_tok = target->owner_tok;
    }
  }
  mk_cpp_member_pointer(&member_type, member_owner_tok);
  if (NODATA_WANTED) {
    /* The expression is a constant prvalue. There is no backing object in a
       no-output probe, so an lvalue placeholder would incorrectly reject
       constant initialization of aggregates containing member addresses. */
    vset(&member_type, VT_CONST, 0);
    vtop->sym = sym_push2(&global_stack, anon_sym++, member_type.t, 0);
    vtop->sym->type = member_type;
    vtop->sym->cpp_member_address_tok = function ? function->v : member_tok;
    forming_cpp_member_address = 0;
    return 1;
  }
  location = section_add(data_section, sizeof(words), PTR_SIZE);
  memcpy(data_section->data + location, words, sizeof(words));
  if (function && !vm)
  {
    function->a.addrtaken = 1;
    greloca(data_section, function, location, R_DATA_PTR, 0);
  }
  /* Constant member addresses can be retained by template arguments beyond
     the local declaration scope in which they were first formed. */
  saved_local = local_stack;
  local_stack = NULL;
  vpush_ref(&member_type, data_section, location, sizeof(words));
  local_stack = saved_local;
  vtop->sym->cpp_member_address_tok = function ? function->v : member_tok;
  vtop->r |= VT_LVAL;
  forming_cpp_member_address = 0;
  return 1;
}

static void cpp_member_pointer_load_word(int address_slot, int offset)
{
  vset(&char_pointer_type, VT_LOCAL | VT_LVAL, address_slot);
  if (offset) { vpushi(offset); gen_op('+'); }
  gv(RC_INT);
  vtop->type.t = VT_LLONG;
  vtop->type.ref = NULL;
  vtop->r |= VT_LVAL;
}

static void cpp_member_pointer_save_top(CType *type, int slot)
{
  vset(type, VT_LOCAL | VT_LVAL, slot);
  vswap();
  vstore();
  vpop();
}

static int cpp_member_pointer_address_slot(void)
{
  int slot;
  cprime_warning("member-pointer address flags %x", vtop->r);
  /* A member-pointer conversion may start from `&Derived::member`, which is
     a prvalue in the lowered two-word representation.  Consumers below read
     both words through an address, so give that prvalue ordinary temporary
     storage before forming the address. */
  if (!(vtop->r & VT_LVAL))
  {
    CType storage_type = vtop->type;
    SValue value;
    int align, size, address, r2;
    storage_type.t &= ~(VT_CONSTANT | VT_VOLATILE);
    size = type_size(&storage_type, &align);
    address = get_temp_local_var(size, align, &r2);
    cpp_temp_reserve_storage(address, size, r2);
    value = *vtop--;
    vset(&storage_type, VT_LOCAL | VT_LVAL, address);
    vtop->r2 = r2;
    vpushv(&value);
    vstore();
    vpop();
    vset(&storage_type, VT_LOCAL | VT_LVAL, address);
    vtop->r2 = r2;
  }
  slot = loc = (loc - PTR_SIZE) & -PTR_SIZE;
  mk_pointer(&vtop->type);
  gaddrof();
  vtop->type = char_pointer_type;
  cpp_member_pointer_save_top(&char_pointer_type, slot);
  return slot;
}

static int convert_cpp_member_pointer(CType *type, int explicit_cast)
{
  CppMemberPointerType *target = find_cpp_member_pointer_type(type);
  CppMemberPointerType *source = find_cpp_member_pointer_type(&vtop->type);
  int offset, source_slot, result_slot, skip_adjustment;
  CType writable_type, word_type;
  if (target && source && source->deferred)
  {
    if (!resolve_deferred_cpp_member_address(type))
      return 0;
    source = find_cpp_member_pointer_type(&vtop->type);
  }
  if (!target)
  {
    if (source && (type->t & VT_BTYPE) == VT_BOOL)
    {
      source_slot = cpp_member_pointer_address_slot();
      cpp_member_pointer_load_word(source_slot, 0);
      cpp_member_pointer_load_word(source_slot, PTR_SIZE);
      gen_op('|');
      vpushi(0); gen_op(TOK_NE);
      vtop->type = *type;
      return 1;
    }
    return 0;
  }
  if (!source)
  {
    size_t location;
    if ((vtop->type.t & VT_BTYPE) == VT_STRUCT
        && try_call_cpp_conversion_operator(type, explicit_cast != 0))
      return convert_cpp_member_pointer(type, explicit_cast);
    if (!is_null_pointer(vtop))
      cast_error(&vtop->type, type);
    vpop();
    location = section_add(data_section, 2 * PTR_SIZE, PTR_SIZE);
    memset(data_section->data + location, 0, 2 * PTR_SIZE);
    vpush_ref(type, data_section, location, 2 * PTR_SIZE);
    vtop->r |= VT_LVAL;
    return 1;
  }
  if (source == target)
  {
    vtop->type = *type;
    return 1;
  }
  if (!is_compatible_unqualified_types(&target->member_type, &source->member_type)
      || ((source->member_type.t & ~target->member_type.t) & (VT_CONSTANT | VT_VOLATILE)))
  {
    /* [expr.reinterpret.cast] permits an explicit conversion between two
       pointer-to-member types that are both function types or both object
       types.  Both members are stored as a target/adjustment pair, so the
       words carry over unchanged. */
    int source_is_func = (source->member_type.t & VT_BTYPE) == VT_FUNC;
    int target_is_func = (target->member_type.t & VT_BTYPE) == VT_FUNC;
    if (!explicit_cast || source_is_func != target_is_func)
      cast_error(&vtop->type, type);
    vtop->type = *type;
    return 1;
  }
  if (!class_subobject_offset(target->owner_tok, source->owner_tok, &offset))
  {
    if (!class_subobject_offset(source->owner_tok, target->owner_tok, &offset))
    {
      /* reinterpret_cast admits member pointers of unrelated classes as long
         as both members are object types or both are function types; the two
         words carry over unchanged. */
      if (explicit_cast < 2)
        cast_error(&vtop->type, type);
      vtop->type = *type;
      return 1;
    }
    if (!explicit_cast)
      cast_error(&vtop->type, type);
    offset = -offset;
  }
  else if (explicit_cast < 2
           && class_virtual_base_path(target->owner_tok, source->owner_tok))
  {
    /* [conv.mem]: a pointer to member of base `source` converts to a pointer
       to member of derived `target` only when that base is non-virtual.  A
       virtual base subobject has no fixed offset, so the adjustment the
       conversion would need cannot be computed. */
    cast_error(&vtop->type, type);
  }
  if (!offset) { vtop->type = *type; return 1; }
  source_slot = cpp_member_pointer_address_slot();
  result_slot = loc = (loc - 2 * PTR_SIZE) & -PTR_SIZE;
  word_type.t = VT_LLONG;
  word_type.ref = NULL;
  cpp_member_pointer_load_word(source_slot, 0);
  cpp_member_pointer_save_top(&word_type, result_slot);
  cpp_member_pointer_load_word(source_slot, PTR_SIZE);
  cpp_member_pointer_save_top(&word_type, result_slot + PTR_SIZE);
  if (offset)
  {
    vset(&word_type, VT_LOCAL | VT_LVAL, result_slot);
    vset(&word_type, VT_LOCAL | VT_LVAL, result_slot + PTR_SIZE);
    gen_op('|');
    skip_adjustment = gvtst(1, 0);
    vset(&word_type, VT_LOCAL | VT_LVAL, result_slot + PTR_SIZE);
    vpush64(VT_LLONG, (long long)offset * 2);
    gen_op('+');
    cpp_member_pointer_save_top(&word_type, result_slot + PTR_SIZE);
    gsym(skip_adjustment);
  }
  writable_type = *type;
  writable_type.t &= ~(VT_CONSTANT | VT_VOLATILE);
  vset(&writable_type, VT_LOCAL | VT_LVAL, result_slot);
  return 1;
}

static int compare_cpp_member_pointers(int op)
{
  CppMemberPointerType *left = find_cpp_member_pointer_type(&vtop[-1].type);
  CppMemberPointerType *right = find_cpp_member_pointer_type(&vtop->type);
  CType common;
  int left_slot, right_slot;
  if (!left && !right)
    return 0;
  if (op != TOK_EQ && op != TOK_NE)
    cprime_error("member pointers support equality comparison only");
  common = left ? vtop[-1].type : vtop->type;
  if (left && right && left != right
      && class_has_base(right->owner_tok, left->owner_tok))
    common = vtop->type;
  /* The composite member-pointer type keeps the union of both members'
     cv-qualifiers, so a more-qualified operand converts by adding
     qualification instead of being rejected for dropping it. */
  if (left && right && left != right)
  {
    CppMemberPointerType *common_member = find_cpp_member_pointer_type(&common);
    CppMemberPointerType *other = common_member == left ? right : left;
    int qualifiers;
    if (!common_member)
      cast_error(&vtop->type, &vtop[-1].type);
    qualifiers = other->member_type.t & (VT_CONSTANT | VT_VOLATILE);
    if (qualifiers & ~common_member->member_type.t)
    {
      CType merged = common_member->member_type;
      int storage = common.t & VT_STORAGE;
      merged.t |= qualifiers;
      mk_cpp_member_pointer(&merged, common_member->owner_tok);
      merged.t |= storage;
      common = merged;
    }
  }
  convert_cpp_member_pointer(&common, 0);
  right_slot = cpp_member_pointer_address_slot();
  convert_cpp_member_pointer(&common, 0);
  left_slot = cpp_member_pointer_address_slot();
  cpp_member_pointer_load_word(left_slot, 0);
  cpp_member_pointer_load_word(right_slot, 0);
  gen_op('^');
  cpp_member_pointer_load_word(left_slot, PTR_SIZE);
  cpp_member_pointer_load_word(right_slot, PTR_SIZE);
  gen_op('^');
  gen_op('|');
  vpushi(0); gen_op(op);
  return 1;
}

static void bind_cpp_member_pointer(int is_arrow)
{
  CppMemberPointerType *entry;
  CType object_type, owner_pointer, call_type, function_pointer;
  Sym *prototype, *receiver, *saved_local;
  int receiver_slot, member_slot, target_slot, skip_virtual;
  int object_cv;
  {
    /* The built-in operator applies to a class object (`.*`) or a class
       pointer (`->*`). Any other left operand selects an overloaded
       operator, which owns the right operand's parse. */
    int left_bt = vtop->type.t & VT_BTYPE;
    int builtin_applies = is_arrow
      ? (left_bt == VT_PTR
         && (pointed_type(&vtop->type)->t & VT_BTYPE) == VT_STRUCT)
      : left_bt == VT_STRUCT;
    if (!builtin_applies)
    {
      /* With no operator->* declared anywhere in the translation unit, the
         built-in candidate is the only one left, and it accepts a class
         operand through its conversion to a pointer to the member's owner
         class (`Pointer` declaring `operator Item *`).  Do this before the
         overload attempt consumes the right operand: that attempt cannot
         rewind the token stream when no candidate applies. */
      if (is_arrow && is_cpp_translation_unit()
          && (vtop->type.t & VT_BTYPE) == VT_STRUCT)
      {
        int operator_tok = tok_alloc_const("operator->*");
        if (!free_func_candidates(operator_tok)
            && !resolve_member_func(&vtop->type, operator_tok))
        {
          try_cpp_pointer_conversion(0);
          builtin_applies = (vtop->type.t & VT_BTYPE) == VT_PTR
            && (pointed_type(&vtop->type)->t & VT_BTYPE) == VT_STRUCT;
        }
      }
      if (!builtin_applies && is_cpp_translation_unit()
          && try_call_cpp_member_access_operator(is_arrow))
        return;
      if (!builtin_applies)
        cprime_error(is_arrow ? "class pointer required for ->* operator"
                              : "object required for .* operator");
    }
  }
  if (is_arrow) try_cpp_pointer_conversion(0);
  if (!is_arrow)
  {
    if ((vtop->type.t & VT_BTYPE) != VT_STRUCT)
      cprime_error("object required for .* operator");
    mk_pointer(&vtop->type);
    gaddrof();
  }
  if ((vtop->type.t & VT_BTYPE) != VT_PTR
      || (pointed_type(&vtop->type)->t & VT_BTYPE) != VT_STRUCT)
    cprime_error("class pointer required for ->* operator");
  object_type = vtop->type;
  object_cv = pointed_type(&object_type)->t & (VT_CONSTANT | VT_VOLATILE);
  receiver_slot = loc = (loc - PTR_SIZE) & -PTR_SIZE;
  cpp_member_pointer_save_top(&object_type, receiver_slot);
  unary();
  entry = find_cpp_member_pointer_type(&vtop->type);
  if (!entry)
    type_incompatibility_error(&vtop->type, &object_type,
      "right operand of member access has type '%s', not a member pointer for '%s'");
  mk_pointer(&vtop->type);
  gaddrof();
  vtop->type = char_pointer_type;
  member_slot = loc = (loc - PTR_SIZE) & -PTR_SIZE;
  cpp_member_pointer_save_top(&char_pointer_type, member_slot);
  make_class_type_from_tok(&owner_pointer, entry->owner_tok);
  owner_pointer.t |= object_cv;
  mk_pointer(&owner_pointer);
  vset(&object_type, VT_LOCAL | VT_LVAL, receiver_slot);
  if (get_struct_type_name_tok(pointed_type(&object_type)) != entry->owner_tok
      && !try_adjust_derived_pointer_to_base(&owner_pointer))
    cprime_error("pointer-to-member object has incompatible class");
  vtop->type = char_pointer_type;
  cpp_member_pointer_load_word(member_slot, PTR_SIZE);
  vpushi(1); gen_op(TOK_SAR);
  gen_op('+');
  vtop->type = owner_pointer;
  cpp_member_pointer_save_top(&owner_pointer, receiver_slot);
  if ((entry->member_type.t & VT_BTYPE) != VT_FUNC)
  {
    vset(&char_pointer_type, VT_LOCAL | VT_LVAL, receiver_slot);
    cpp_member_pointer_load_word(member_slot, 0);
    vpushi(1); gen_op('-');
    gen_op('+');
    vtop->type = entry->member_type;
    vtop->type.t |= object_cv;
    vtop->r |= VT_LVAL;
    return;
  }
  if ((object_cv & VT_CONSTANT) && !(entry->member_type.t & VT_CONSTANT))
    cprime_error("non-const member function requires a non-const object");
  call_type = entry->member_type;
  saved_local = local_stack;
  local_stack = NULL;
  prototype = sym_push(SYM_FIELD, &call_type.ref->type, 0, 0);
  prototype->f = call_type.ref->f;
  receiver = sym_push(SYM_FIELD, &owner_pointer, 0, 0);
  receiver->next = call_type.ref->next;
  prototype->next = receiver;
  local_stack = saved_local;
  call_type.ref = prototype;
  function_pointer = call_type;
  mk_pointer(&function_pointer);
  target_slot = loc = (loc - PTR_SIZE) & -PTR_SIZE;
  cpp_member_pointer_load_word(member_slot, 0);
  vtop->type = function_pointer;
  cpp_member_pointer_save_top(&function_pointer, target_slot);
  cpp_member_pointer_load_word(member_slot, PTR_SIZE);
  vpushi(1); gen_op('&');
  skip_virtual = gvtst(1, 0);
  vset(&char_pointer_type, VT_LOCAL | VT_LVAL, receiver_slot);
  cpp_member_pointer_load_word(member_slot, 0);
  vtop->type.t |= VT_UNSIGNED;
  vpushi(32); gen_op(TOK_SHR);
  gen_op('+');
  vtop->type = char_pointer_type;
  vtop->r |= VT_LVAL;
  gv(RC_INT);
  cpp_member_pointer_load_word(member_slot, 0);
  vpush64(VT_LLONG | VT_UNSIGNED, 0xffffffffu); gen_op('&');
  vpushi(PTR_SIZE); gen_op('*');
  gen_op('+');
  vtop->type = function_pointer;
  vtop->r |= VT_LVAL;
  cpp_member_pointer_save_top(&function_pointer, target_slot);
  gsym(skip_virtual);
  vset(&function_pointer, VT_LOCAL | VT_LVAL, target_slot);
  vtop->bound_member_receiver = receiver_slot;
}
