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
  storage_type.t = VT_STRUCT;
  storage_type.ref = NULL;
  symbol = sym_push(type_tok | SYM_STRUCT, &storage_type, PTR_SIZE, 2 * PTR_SIZE);
  symbol->sym_scope = 0;
  symbol->type.ref = symbol;
  word_type.t = VT_LLONG;
  word_type.ref = NULL;
  symbol->next = field = sym_push(tok_alloc_const("__target") | SYM_FIELD,
                                  &word_type, 0, 0);
  field->next = sym_push(tok_alloc_const("__adjustment") | SYM_FIELD,
                         &word_type, 0, PTR_SIZE);
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
  int angle = 0, colons = 0, result = 0;
  if (tok < TOK_UIDENT) return 0;
  replay = tok_str_alloc();
  while (tok != TOK_EOF) {
    if (!angle && tok == '*') { result = colons == 2; break; }
    if (!angle && tok < TOK_UIDENT && tok != ':' && tok != TOK_LT) break;
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
    return 0;
  replay = tok_str_alloc();
  owner = tok_str_alloc();
  for (;;)
  {
    if (tok < TOK_UIDENT)
      break;
    tok_str_add_tok(replay);
    tok_str_add_tok(owner);
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

static int try_form_cpp_member_pointer(int class_tok, int member_tok)
{
  CType owner_type, member_type;
  Sym *field, *function = NULL, *saved_local;
  VirtualMethodInfo *vm = NULL;
  int offset = 0;
  size_t location;
  uint64_t words[2] = {0, 0};
  if (!make_class_type_from_tok(&owner_type, class_tok))
    return 0;
  if (class_has_static_member_func(class_tok, member_tok))
    return 0;
  field = find_field_try(&owner_type, member_tok, &offset);
  if (field && (field->type.t & VT_BTYPE) != VT_FUNC)
  {
    member_type = field->type;
    words[0] = (uint64_t)offset + 1;
  }
  else
  {
    Sym *prototype;
    MemberFuncOverload *candidate;
    if (cpp_member_address_target
        && class_or_inst_has_member_template_name(class_tok, member_tok)) {
      CppMemberPointerType *target = find_cpp_member_pointer_type(cpp_member_address_target);
      int saved_explicit = explicit_member_template_arg_tok, count = 0;
      CType arguments[CPC_MAX_CALL_ARGUMENTS];
      if (tok == TOK_LT || tok == '<') {
        TemplateArgList explicit_arguments;
        parse_template_type_args(&explicit_arguments);
        explicit_member_template_arg_tok = intern_explicit_member_arguments(&explicit_arguments);
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
      explicit_member_template_arg_tok = saved_explicit;
    }
    for (candidate = member_func_candidates(class_tok, member_tok);
         candidate; candidate = candidate->bucket_next)
      if (candidate->struct_tok == class_tok && candidate->method_tok == member_tok)
      {
        Sym *selected = global_symbol_find(candidate->mangled_tok);
        if (!selected) selected = sym_find(candidate->mangled_tok);
        if (!selected) continue;
        if (cpp_member_address_target) {
          CppMemberPointerType *target = find_cpp_member_pointer_type(cpp_member_address_target);
          CType signature = selected->type;
          Sym prototype = *signature.ref;
          if (target && (target->member_type.t & VT_BTYPE) == VT_FUNC) {
            if (prototype.next) {
              signature.t |= pointed_type(&prototype.next->type)->t & (VT_CONSTANT | VT_VOLATILE);
              prototype.next = prototype.next->next;
            }
            signature.ref = &prototype;
            if (target->member_type.ref->f.func_noexcept && !prototype.f.func_noexcept) continue;
            prototype.f.func_noexcept = target->member_type.ref->f.func_noexcept;
            if (!is_compatible_types(&signature, &target->member_type)) continue;
          }
        }
        if (function && function != selected)
          cprime_error("overloaded member address requires a target member-pointer type");
        function = selected;
      }
    if (!function)
      function = resolve_member_func(&owner_type, member_tok);
    if (!function || (function->type.t & VT_BTYPE) != VT_FUNC)
      return 0;
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
  }
  mk_cpp_member_pointer(&member_type, class_tok);
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
  int slot = loc = (loc - PTR_SIZE) & -PTR_SIZE;
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
    cast_error(&vtop->type, type);
  if (!class_subobject_offset(target->owner_tok, source->owner_tok, &offset))
  {
    if (!explicit_cast
        || !class_subobject_offset(source->owner_tok, target->owner_tok, &offset))
      cast_error(&vtop->type, type);
    offset = -offset;
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
