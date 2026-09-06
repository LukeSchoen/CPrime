static void call_lifecycle_constructor_members(CType *type, int r, int addr);
static void tok_str_append(TokenString *dst, TokenString *src);
static void call_lifecycle_constructor_members_base_ptr(CType *type,
                                                        SValue *base_ptr,
                                                        int base_offset);
static void add_pending_lifecycle_func(CType *struct_type, int method_tok,
                                       TokenString *params, TokenString *body,
                                       int internal, int is_defaulted,
                                       int exception_spec, int is_explicit);

static void normalize_lifecycle_void_params(TokenString *params)
{
  int i;
  if (!params)
    return;
  i = tok_str_next_non_linenum_index(params->str, params->len, 0);
  if (i < params->len && params->str[i] == TOK_VOID)
    i = tok_str_next_non_linenum_index(params->str, params->len, i + 1);
  if (i >= params->len || params->str[i] == TOK_EOF)
  {
    /* The lowered signature has an implicit receiver, so retaining a
       source `(void)` would incorrectly produce `(Class *this, void)`. */
    params->len = 0;
    tok_str_add(params, TOK_EOF);
  }
}

static void note_lifecycle_exception_spec(CType *type, CType *function,
                                          int method, int specification)
{
  apply_function_noexcept(function, specification);
  if (method == TOK_DESTRUCTOR1) {
    function->ref->f.func_cxx_destructor = 1;
    if (specification) {
      type->ref->a.dtor_noexcept_specified = 1;
      type->ref->a.dtor_noexcept = specification == 1;
    }
  }
}

/* An implicit or unspecified destructor inherits its exception specification
   from the destructors of its potentially constructed subobjects. */
static int class_destructor_is_noexcept(CType *type)
{
  Sym *field;
  if ((type->t & VT_ARRAY) && type->ref)
    return class_destructor_is_noexcept(pointed_type(type));
  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref) return 1;
  if (type->ref->a.dtor_noexcept_specified)
    return type->ref->a.dtor_noexcept;
  if (IS_UNION(type->t)) return 1;
  for (field = type->ref->next; field; field = field->next)
    if (!class_destructor_is_noexcept(&field->type)) return 0;
  return 1;
}

static CType make_lifecycle_func_type(CType *struct_type)
{
  CType void_type, func_type, this_type;
  Sym *fref, *param;
  Sym *saved_ls;

  void_type.t = VT_VOID;
  void_type.ref = NULL;
  func_type.t = VT_FUNC;
  /* Function-type construction symbols must not leak onto a live local
     scope: an enclosing scope pop would unlink the 'this'-named parameter
     symbols and clobber the token table entry for the real 'this'.  Push
     them on the global stack (as at file scope) instead. */
  saved_ls = local_stack;
  local_stack = NULL;
  func_type.ref = fref = sym_push(SYM_FIELD, &void_type, 0, 0);
  fref->f.func_call = FUNC_CDECL;
  fref->f.func_type = FUNC_NEW;

  this_type = *struct_type;
  this_type.t &= ~(VT_CONSTANT | VT_VOLATILE);
  mk_pointer(&this_type);
  param = sym_push(SYM_FIELD, &this_type, VT_LOCAL | VT_LVAL, 0);
  param->v = tok_alloc_const("this");
  fref->next = param;
  local_stack = saved_ls;
  return func_type;
}

static Sym *declare_lifecycle_func(CType *struct_type, int method_tok)
{
  int struct_tok, mangled_tok;
  CType func_type;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("constructors/destructors require a named struct/class");
  if (method_tok == TOK_CONSTRUCTOR1)
    struct_type->ref->a.lifecycle_ctor = 1;
  else if (method_tok == TOK_DESTRUCTOR1)
    struct_type->ref->a.lifecycle_dtor = 1;
  mangled_tok = make_lifecycle_func_tok_for_params(struct_tok, method_tok, NULL, 1);
  func_type = make_lifecycle_func_type(struct_type);
  note_lifecycle_exception_spec(struct_type, &func_type, method_tok, 0);
  note_member_func_overload(struct_tok, method_tok, mangled_tok, &func_type);
  return external_global_sym(mangled_tok, &func_type);
}

static Sym *resolve_lifecycle_func(CType *type, int method_tok)
{
  int struct_tok, mangled_tok;
  Sym *s;

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return NULL;
  if (method_tok == TOK_CONSTRUCTOR1 && !type->ref->a.lifecycle_ctor)
    return NULL;
  if (method_tok == TOK_DESTRUCTOR1 && !type->ref->a.lifecycle_dtor)
    return NULL;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  instantiate_template_member_for_call(type, method_tok, NULL,
                                       method_tok == TOK_CONSTRUCTOR1 ? 0 : -1);
  mangled_tok = make_lifecycle_func_tok_for_params(struct_tok, method_tok, NULL, 1);
  s = sym_find(mangled_tok);
  if (!s)
    s = global_symbol_find(mangled_tok);
  if (!s)
    return NULL;
  if ((s->type.t & VT_BTYPE) != VT_FUNC)
    cprime_error("lifecycle target '%s' is not declared as function",
              get_tok_str(mangled_tok, NULL));
  return s;
}

static int lifecycle_has_only_this_param(Sym *func_sym)
{
  Sym *param;

  if (!func_sym || (func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    return 0;
  param = func_sym->type.ref->next;
  return param && !param->next;
}

static Sym *resolve_autodtor_func(CType *type)
{
  CType destroying_type = *type;
  destroying_type.t &= ~(VT_CONSTANT | VT_VOLATILE);
  type = &destroying_type;
  Sym *s = resolve_lifecycle_func(type, TOK_DESTRUCTOR1);
  if (!s && is_cpp_translation_unit()
      && (type->t & VT_BTYPE) == VT_STRUCT && type->ref
      && !type->ref->a.lifecycle_dtor && !IS_UNION(type->t)) {
    Sym *field;
    for (field = type->ref->next; field; field = field->next) {
      CType *element = &field->type;
      while ((element->t & VT_ARRAY) && element->ref)
        element = pointed_type(element);
      if (resolve_autodtor_func(element)) {
        TokenString *body = tok_str_alloc();
        s = declare_lifecycle_func(type, TOK_DESTRUCTOR1);
        tok_str_add(body, '{');
        tok_str_add(body, '}');
        tok_str_add(body, TOK_EOF);
        add_pending_lifecycle_func(type, TOK_DESTRUCTOR1, NULL, body, 1, 1, 0, 0);
        tok_str_free(body);
        break;
      }
    }
  }
  if (!lifecycle_has_only_this_param(s))
    return NULL;
  return s;
}

static Sym *resolve_autoctor_func(CType *type)
{
  Sym *s = resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1, 0);
  if (!s)
    s = resolve_lifecycle_func(type, TOK_CONSTRUCTOR1);
  if (!s && is_cpp_translation_unit()
      && (type->t & VT_BTYPE) == VT_STRUCT && type->ref
      && !type->ref->a.lifecycle_ctor && !IS_UNION(type->t)) {
    Sym *field;
    for (field = type->ref->next; field; field = field->next)
      if (field->default_arg) {
        TokenString *body = tok_str_alloc(), *initializers;
        int saved_tok = tok;
        CValue saved_value = tokc;
        /* A default member initializer also belongs to the implicit default
           constructor. Generate its ordinary member-initializer sequence so
           arrays, nested objects and exception cleanups share source ctor
           semantics. Declaring it first prevents recursive synthesis. */
        s = declare_lifecycle_func(type, TOK_CONSTRUCTOR1);
        tok = '{';
        initializers = parse_constructor_member_initializers(type);
        tok = saved_tok;
        tokc = saved_value;
        tok_str_add(body, '{');
        if (initializers) {
          tok_str_append(body, initializers);
          tok_str_free(initializers);
          note_struct_member_init_list(get_struct_type_name_tok(type));
        }
        tok_str_add(body, '}');
        tok_str_add(body, TOK_EOF);
        add_pending_lifecycle_func(type, TOK_CONSTRUCTOR1, NULL, body, 1, 1, 0, 0);
        tok_str_free(body);
        break;
      }
  }
  if (s && !lifecycle_has_only_this_param(s)
      && member_func_min_arg_count(&s->type) > 0)
    return NULL;
  return s;
}

static int class_has_virtual_destructor(CType *type)
{
  ClassBaseInfo *base;
  int name;
  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref) return 0;
  if (type->ref->a.virtual_destructor) return 1;
  name = get_struct_type_name_tok(type);
  for (base = class_base_infos; base; base = base->next)
    if (base->class_tok == name && base->field
        && class_has_virtual_destructor(&base->field->type)) return 1;
  return 0;
}

static void note_class_virtual_destructor(CType *type)
{
  CType result = { VT_VOID, NULL }, function;
  if (!class_has_virtual_destructor(type)) return;
  type->ref->a.virtual_destructor = 1;
  resolve_autodtor_func(type);
  function = make_func_type_from_saved_params(&result, NULL);
  note_virtual_method(type, TOK_DESTRUCTOR1, &function, 0);
}

static void push_void_value(void)
{
  vpushi(0);
  vtop->type.t = VT_VOID;
}

static int is_pseudo_destructor_type_tok(int t)
{
  return t >= TOK_UIDENT || t == TOK_VOID || t == TOK_CHAR || t == TOK_SHORT
         || t == TOK_INT || t == TOK_LONG || t == TOK_FLOAT
         || t == TOK_DOUBLE || t == TOK_BOOL;
}

static void call_explicit_destructor(CType *type, int name_tok)
{
  int struct_tok;
  Sym *func_sym;

  if ((type->t & VT_BTYPE) != VT_STRUCT)
  {
    vpop();
    push_void_value();
    return;
  }

  struct_tok = get_struct_type_name_tok(type);
  if (struct_tok && !class_tok_matches_unqualified_name(struct_tok, name_tok))
  {
    Sym *alias = sym_find(name_tok);
    if (!alias) alias = global_symbol_find(name_tok);
    if (!alias || !(alias->type.t & VT_TYPEDEF)
        || get_struct_type_name_tok(&alias->type) != struct_tok)
      cprime_error("destructor name must match class name");
  }

  /* An explicit destructor call also odr-uses an implicit destructor. Build
     its complete-object member/base destruction just as automatic cleanup
     does, including classes first used through placement construction. */
  func_sym = resolve_autodtor_func(type);
  if (!func_sym)
  {
    vpop();
    push_void_value();
    return;
  }
  if ((func_sym->type.t & VT_BTYPE) != VT_FUNC || !func_sym->type.ref)
    cprime_error("destructor target is not declared as function");
  if (!lifecycle_has_only_this_param(func_sym))
    cprime_error("explicit destructor call requires destructor with no parameters");

  vtop->type.t &= ~(VT_CONSTANT | VT_VOLATILE);
  mk_pointer(&vtop->type);
  gaddrof();
  vpushsym(&func_sym->type, func_sym);
  vswap();
  finish_cpp_member_func_call(func_sym->type.ref, 1);
}

static void call_lifecycle_constructor_raw(CType *type, int r, int addr, Sym *sym,
                                           Sym *ctor_func)
{
  /* Constructors address the unqualified object during its lifetime start. */
  CType constructing_type = *type;
  constructing_type.t &= ~(VT_CONSTANT | VT_VOLATILE);
  type = &constructing_type;
  Sym *func_type, *sa;
  TokenString *call_args[32];
  CType call_arg_types[32];
  int nb_args, call_arg_count, ai;
  int implicit_copy = 0;

  skip('(');
  skip_line_markers();
  call_arg_count = count_saved_call_args(call_args, 32);
  infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
  {
    Sym *overload = resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                     call_arg_types,
                                                     call_arg_count);
    if (call_arg_count == 1)
    {
      CType source = call_arg_types[0];
      Sym *copy_constructor;
      if (is_reference_type(&source)) source = *pointed_type(&source);
      source.t &= ~VT_RVALUE_REFERENCE;
      if (is_compatible_unqualified_types(type, &source))
      {
        copy_constructor = resolve_copy_constructor_func(type, &call_arg_types[0]);
        if (copy_constructor) overload = copy_constructor;
        else implicit_copy = 1;
      }
    }
    if (!overload)
      overload = resolve_member_field_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                        call_arg_types,
                                                        call_arg_count);
    if (!overload)
      overload = resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1,
                                                  call_arg_count);
    if (overload)
      ctor_func = overload;
  }
  if (!implicit_copy
      && (!ctor_func || (ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref))
    cprime_error("constructor target is not declared as function");

  if (call_arg_count == 1
      && (implicit_copy || is_defaulted_lifecycle_constructor(type, ctor_func)))
  {
    init_params p = {0};
    TokenString macro = *call_args[0];
    TokenString *saved_macro_stack = macro_stack;
    const int *saved_macro_ptr = macro_ptr;
    begin_macro(&macro, 0);
    next();
    expr_eq();
    while (macro_stack && macro_stack != saved_macro_stack)
      end_macro();
    macro_ptr = saved_macro_ptr;
    init_putv(&p, type, addr);
    next();
    return;
  }

  call_lifecycle_constructor_members(type, r, addr);
  save_lvalues();
  vpushsym(&ctor_func->type, ctor_func);
  vset(type, r, addr);
  if (sym)
    vtop->sym = sym;
  mk_pointer(&vtop->type);
  gaddrof();

  func_type = ctor_func->type.ref;
  sa = func_type->next;
  gfunc_param_typed(func_type, sa);
  if (sa)
    sa = sa->next;
  nb_args = 1;

  for (ai = 0; ai < call_arg_count; ++ai)
  {
    emit_saved_arg_for_param(call_args[ai], sa);
    tok_str_free(call_args[ai]);
    gfunc_param_typed_with_conversions(func_type, sa);
    nb_args++;
    if (sa)
      sa = sa->next;
  }
  while (sa)
  {
    emit_default_arg(func_type, sa);
    nb_args++;
    sa = sa->next;
  }
  next();
  vcheck_cmp();
  gfunc_call(nb_args);
  drop_leaked_call_target(ctor_func);
}

static void call_lifecycle_constructor_noargs(CType *type, int r, int addr, Sym *sym,
                                              Sym *ctor_func)
{
  /* Constructors address the unqualified object during its lifetime start. */
  CType constructing_type = *type;
  constructing_type.t &= ~(VT_CONSTANT | VT_VOLATILE);
  type = &constructing_type;
  Sym *func_type, *sa;
  int nb_args, saved_tok;
  CValue saved_tokc;

  if (!ctor_func || (ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref)
    cprime_error("constructor target is not declared as function");

  save_lvalues();
  vpushsym(&ctor_func->type, ctor_func);
  vset(type, r, addr);
  if (sym)
    vtop->sym = sym;
  mk_pointer(&vtop->type);
  gaddrof();

  func_type = ctor_func->type.ref;
  sa = func_type->next;
  gfunc_param_typed(func_type, sa);
  if (sa)
    sa = sa->next;
  nb_args = 1;
  saved_tok = tok;
  saved_tokc = tokc;

  while (sa)
  {
    emit_default_arg(func_type, sa);
    nb_args++;
    sa = sa->next;
  }
  tok = saved_tok;
  tokc = saved_tokc;

  gfunc_call(nb_args);
  drop_leaked_call_target(ctor_func);
}

static void call_lifecycle_constructor_members(CType *type, int r, int addr)
{
  Sym *field, *ctor_func;
  int i, align, elem_size;
  int struct_tok;
  CType *elem_type;

  if ((r & VT_VALMASK) == VT_LLOCAL) {
    SValue pointer;
    vset(type, r, addr);
    mk_pointer(&vtop->type); gaddrof();
    pointer = *vtop--;
    call_lifecycle_constructor_members_base_ptr(type, &pointer, 0);
    return;
  }

  if ((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
      && type->ref && type->ref->c > 0)
  {
    elem_type = pointed_type(type);
    elem_size = type_size(elem_type, &align);
    if (elem_size < 0)
      return;
    for (i = 0; i < type->ref->c; ++i)
    {
      call_lifecycle_constructor_members(elem_type, r, addr + i * elem_size);
      ctor_func = resolve_autoctor_func(elem_type);
      if (ctor_func)
        call_lifecycle_constructor_noargs(elem_type, r, addr + i * elem_size,
                                          NULL, ctor_func);
    }
    return;
  }

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return;
  struct_tok = get_struct_type_name_tok(type);
  if (struct_tok && struct_has_member_init_list(struct_tok))
    return;

  for (field = type->ref->next; field; field = field->next)
  {
    if ((field->type.t & VT_BTYPE) != VT_STRUCT
        && !((field->type.t & VT_ARRAY)
             && (field->type.t & VT_BTYPE) == VT_PTR))
      continue;

    call_lifecycle_constructor_members(&field->type, r, addr + field->c);
    ctor_func = resolve_autoctor_func(&field->type);
    if (ctor_func)
      call_lifecycle_constructor_noargs(&field->type, r, addr + field->c, NULL, ctor_func);
  }
}

static void push_field_lvalue_from_base_ptr(SValue *base_ptr, CType *field_type,
                                            int offset)
{
  vpushv(base_ptr);
  if (vtop->r & VT_LVAL)
    gv(RC_INT);
  vtop->type = char_pointer_type;
  vpushi(offset);
  gen_op('+');
  vtop->type = *field_type;
  if (!(vtop->type.t & VT_ARRAY) && (vtop->type.t & VT_BTYPE) != VT_FUNC)
    vtop->r |= VT_LVAL;
  maybe_indir_reference();
}

static void spill_pointer_svalue_to_local(SValue *ptr)
{
  CType ptr_type = ptr->type;
  SValue local_slot;
  int addr;

  loc = (loc - PTR_SIZE) & -PTR_SIZE;
  addr = loc;
  vset(&ptr_type, VT_LOCAL | VT_LVAL, addr);
  local_slot = *vtop;
  vpushv(ptr);
  vstore();
  *ptr = local_slot;
  vtop--;
}

static void stabilize_pointer_svalue_pair(SValue *dst_ptr, SValue *src_ptr)
{
  vpushv(dst_ptr);
  vpushv(src_ptr);
  save_regs(0);
  *dst_ptr = vtop[-1];
  *src_ptr = vtop[0];
  vtop -= 2;
}

static void call_lifecycle_constructor_noargs_expr(CType *type, Sym *ctor_func)
{
  Sym *func_type, *sa;
  int nb_args = 1, saved_tok = tok;
  CValue saved_tokc = tokc;

  if (!ctor_func)
    return;
  if ((ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref)
    cprime_error("constructor target is not declared as function");

  vtop->type.t &= ~(VT_CONSTANT | VT_VOLATILE);
  mk_pointer(&vtop->type);
  gaddrof();
  vpushsym(&ctor_func->type, ctor_func);
  vswap();

  func_type = ctor_func->type.ref;
  sa = func_type->next;
  gfunc_param_typed(func_type, sa);
  if (sa)
    sa = sa->next;
  while (sa)
  {
    emit_default_arg(func_type, sa);
    ++nb_args;
    sa = sa->next;
  }
  tok = saved_tok;
  tokc = saved_tokc;
  gfunc_call(nb_args);
  drop_leaked_call_target(ctor_func);
}

static void call_lifecycle_constructor_members_base_ptr(CType *type,
                                                        SValue *base_ptr,
                                                        int base_offset)
{
  Sym *field, *ctor_func;
  int i, align, elem_size;
  int struct_tok;
  CType *elem_type;
  SValue stable_base_ptr;

  /* A placement expression may leave its destination address only in a
     volatile return register.  Recursive default construction can make one
     call per member, so the first member constructor must not invalidate the
     base used for the remaining members. */
  if (((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR)
      || (type->t & VT_BTYPE) == VT_STRUCT)
  {
    stable_base_ptr = *base_ptr;
    spill_pointer_svalue_to_local(&stable_base_ptr);
    base_ptr = &stable_base_ptr;
  }

  if ((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
      && type->ref && type->ref->c > 0)
  {
    elem_type = pointed_type(type);
    elem_size = type_size(elem_type, &align);
    if (elem_size < 0)
      return;
    for (i = 0; i < type->ref->c; ++i)
    {
      int elem_offset = base_offset + i * elem_size;
      call_lifecycle_constructor_members_base_ptr(elem_type, base_ptr, elem_offset);
      ctor_func = resolve_autoctor_func(elem_type);
      if (ctor_func)
      {
        push_field_lvalue_from_base_ptr(base_ptr, elem_type, elem_offset);
        call_lifecycle_constructor_noargs_expr(elem_type, ctor_func);
      }
    }
    return;
  }

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return;
  struct_tok = get_struct_type_name_tok(type);
  if (struct_tok && struct_has_member_init_list(struct_tok))
    return;

  for (field = type->ref->next; field; field = field->next)
  {
    int field_offset;
    if ((field->type.t & VT_BTYPE) != VT_STRUCT
        && !((field->type.t & VT_ARRAY)
             && (field->type.t & VT_BTYPE) == VT_PTR))
      continue;

    field_offset = base_offset + field->c;
    call_lifecycle_constructor_members_base_ptr(&field->type, base_ptr, field_offset);
    ctor_func = resolve_autoctor_func(&field->type);
    if (ctor_func)
    {
      push_field_lvalue_from_base_ptr(base_ptr, &field->type, field_offset);
      call_lifecycle_constructor_noargs_expr(&field->type, ctor_func);
    }
  }
}

static int struct_needs_memberwise_assignment(CType *type)
{
  Sym *field;
  CType *elem_type;
  int method_tok, align, elem_size;

  if ((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
      && type->ref && type->ref->c > 0)
  {
    elem_type = pointed_type(type);
    return struct_needs_memberwise_assignment(elem_type);
  }

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return 0;

  method_tok = tok_alloc_const("operator=");
  if (resolve_member_func(type, method_tok))
    return 1;

  for (field = type->ref->next; field; field = field->next)
  {
    if ((field->type.t & VT_ARRAY) && (field->type.t & VT_BTYPE) == VT_PTR
        && field->type.ref && field->type.ref->c > 0)
    {
      elem_type = pointed_type(&field->type);
      elem_size = type_size(elem_type, &align);
      if (elem_size >= 0 && struct_needs_memberwise_assignment(elem_type))
        return 1;
      continue;
    }
    if (struct_needs_memberwise_assignment(&field->type))
      return 1;
  }
  return 0;
}

static void assign_one_lvalue_pair(CType *type, SValue *dst_ptr, SValue *src_ptr,
                                   int offset)
{
  SValue *saved_vtop = vtop;
  push_field_lvalue_from_base_ptr(dst_ptr, type, offset);
  push_field_lvalue_from_base_ptr(src_ptr, type, offset);
  if (!try_call_cpp_assignment_operator())
    vstore();
  vtop = saved_vtop;
}

static void assign_struct_memberwise_from_base_ptr(CType *type, SValue *dst_ptr,
                                                   SValue *src_ptr,
                                                   int base_offset)
{
  Sym *field;
  CType *elem_type;
  int i, align, elem_size;
  SValue stable_dst_ptr, stable_src_ptr;

  if (((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
       && type->ref && type->ref->c > 0)
      || ((type->t & VT_BTYPE) == VT_STRUCT && type->ref))
  {
    stable_dst_ptr = *dst_ptr;
    stable_src_ptr = *src_ptr;
    stabilize_pointer_svalue_pair(&stable_dst_ptr, &stable_src_ptr);
    dst_ptr = &stable_dst_ptr;
    src_ptr = &stable_src_ptr;
  }

  if ((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
      && type->ref && type->ref->c > 0)
  {
    elem_type = pointed_type(type);
    elem_size = type_size(elem_type, &align);
    if (elem_size < 0)
      return;
    for (i = 0; i < type->ref->c; ++i)
    {
      int elem_offset = base_offset + i * elem_size;
      if (elem_type->t & VT_ARRAY)
        assign_struct_memberwise_from_base_ptr(elem_type, dst_ptr, src_ptr, elem_offset);
      else
        assign_one_lvalue_pair(elem_type, dst_ptr, src_ptr, elem_offset);
    }
    return;
  }

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
  {
    assign_one_lvalue_pair(type, dst_ptr, src_ptr, base_offset);
    return;
  }

  for (field = type->ref->next; field; field = field->next)
  {
    int field_offset = base_offset + field->c;
    if (field->type.t & VT_ARRAY)
      assign_struct_memberwise_from_base_ptr(&field->type, dst_ptr, src_ptr, field_offset);
    else
      assign_one_lvalue_pair(&field->type, dst_ptr, src_ptr, field_offset);
  }
}

static int struct_needs_memberwise_copy(CType *type)
{
  Sym *field;
  CType *elem_type;
  int align, elem_size;

  if ((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
      && type->ref && type->ref->c > 0)
  {
    elem_type = pointed_type(type);
    return struct_needs_memberwise_copy(elem_type);
  }

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return 0;

  {
    CType move_type = *type;
    move_type.t |= VT_RVALUE_REFERENCE;
    if (resolve_copy_constructor_func(type, type)
        || resolve_copy_constructor_func(type, &move_type))
      return 1;
  }

  for (field = type->ref->next; field; field = field->next)
  {
    if ((field->type.t & VT_ARRAY) && (field->type.t & VT_BTYPE) == VT_PTR
        && field->type.ref && field->type.ref->c > 0)
    {
      elem_type = pointed_type(&field->type);
      elem_size = type_size(elem_type, &align);
      if (elem_size >= 0 && struct_needs_memberwise_copy(elem_type))
        return 1;
      continue;
    }
    if (struct_needs_memberwise_copy(&field->type))
      return 1;
  }
  return 0;
}

static void copy_construct_one_lvalue_pair(CType *type, SValue *dst_ptr,
                                           SValue *src_ptr, int offset,
                                           int move_source)
{
  SValue *saved_vtop = vtop;
  Sym *ctor_func, *ctor_type, *sa;
  CType source_type = *type;
  SValue stable_dst = *dst_ptr, stable_src = *src_ptr;

  /* A reference-returning expression can leave the source address in a
     register. Both addresses are held outside the value stack here; preserve
     them before receiver setup or member constructors reuse that register. */
  stabilize_pointer_svalue_pair(&stable_dst, &stable_src);
  dst_ptr = &stable_dst;
  src_ptr = &stable_src;

  if (is_reference_type(type))
  {
    CType reference_storage = *type;
    /* Copy/move construction binds the destination reference to the same
       object; assignment through a reference is a different operation. */
    decay_reference_type(&reference_storage);
    assign_one_lvalue_pair(&reference_storage, dst_ptr, src_ptr, offset);
    return;
  }

  if (move_source)
    source_type.t |= VT_RVALUE_REFERENCE;
  ctor_func = resolve_copy_constructor_func(type, &source_type);
  if (!ctor_func)
  {
    assign_one_lvalue_pair(type, dst_ptr, src_ptr, offset);
    return;
  }
  if ((ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref)
    cprime_error("copy constructor target is not declared as function");

  call_lifecycle_constructor_members_base_ptr(type, dst_ptr, offset);
  push_field_lvalue_from_base_ptr(dst_ptr, type, offset);
  mk_pointer(&vtop->type);
  gaddrof();
  vpushsym(&ctor_func->type, ctor_func);
  vswap();

  ctor_type = ctor_func->type.ref;
  sa = ctor_type->next;
  gfunc_param_typed(ctor_type, sa);
  if (sa)
    sa = sa->next;

  push_field_lvalue_from_base_ptr(src_ptr, type, offset);
  if (move_source)
    vtop->type.t |= VT_RVALUE_REFERENCE;
  gfunc_param_typed(ctor_type, sa);
  if (sa)
    sa = sa->next;
  if (sa)
    cprime_error("too few arguments to copy constructor");

  vcheck_cmp();
  gfunc_call(2);
  vtop = saved_vtop;
}

static void copy_construct_struct_memberwise_from_base_ptr(CType *type,
                                                           SValue *dst_ptr,
                                                           SValue *src_ptr,
                                                           int base_offset,
                                                           int move_source)
{
  Sym *field;
  CType *elem_type;
  int i, align, elem_size;
  SValue stable_dst_ptr, stable_src_ptr;

  if (((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
       && type->ref && type->ref->c > 0)
      || ((type->t & VT_BTYPE) == VT_STRUCT && type->ref))
  {
    stable_dst_ptr = *dst_ptr;
    stable_src_ptr = *src_ptr;
    stabilize_pointer_svalue_pair(&stable_dst_ptr, &stable_src_ptr);
    dst_ptr = &stable_dst_ptr;
    src_ptr = &stable_src_ptr;
  }

  if ((type->t & VT_ARRAY) && (type->t & VT_BTYPE) == VT_PTR
      && type->ref && type->ref->c > 0)
  {
    elem_type = pointed_type(type);
    elem_size = type_size(elem_type, &align);
    if (elem_size < 0)
      return;
    for (i = 0; i < type->ref->c; ++i)
    {
      int elem_offset = base_offset + i * elem_size;
      CType source_type = *elem_type;
      if (move_source)
        source_type.t |= VT_RVALUE_REFERENCE;
      if ((elem_type->t & VT_ARRAY)
          || ((elem_type->t & VT_BTYPE) == VT_STRUCT
              && !resolve_copy_constructor_func(elem_type, &source_type)))
        copy_construct_struct_memberwise_from_base_ptr(elem_type, dst_ptr,
                                                       src_ptr, elem_offset,
                                                       move_source);
      else
        copy_construct_one_lvalue_pair(elem_type, dst_ptr, src_ptr,
                                       elem_offset, move_source);
    }
    return;
  }

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
  {
    copy_construct_one_lvalue_pair(type, dst_ptr, src_ptr, base_offset,
                                   move_source);
    return;
  }

  for (field = type->ref->next; field; field = field->next)
  {
    int field_offset = base_offset + field->c;
    CType source_type = field->type;
    if (move_source)
      source_type.t |= VT_RVALUE_REFERENCE;
    if ((field->type.t & VT_ARRAY)
        || ((field->type.t & VT_BTYPE) == VT_STRUCT
            && !resolve_copy_constructor_func(&field->type, &source_type)))
      copy_construct_struct_memberwise_from_base_ptr(&field->type, dst_ptr,
                                                     src_ptr, field_offset,
                                                     move_source);
    else
      copy_construct_one_lvalue_pair(&field->type, dst_ptr, src_ptr,
                                     field_offset, move_source);
  }
}

static void register_struct_cleanups(CType *type, Sym *sym)
{
  Sym *field, *cleanup_func, *cls, *field_sym;
  int i, align, elem_size;
  CType *elem_type;

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return;

  for (field = type->ref->next; field; field = field->next)
  {
    if ((field->type.t & VT_ARRAY) && (field->type.t & VT_BTYPE) == VT_PTR
        && field->type.ref && field->type.ref->c > 0)
    {
      elem_type = pointed_type(&field->type);
      elem_size = type_size(elem_type, &align);
      if (elem_size < 0)
        continue;
      for (i = field->type.ref->c - 1; i >= 0; --i)
      {
        CType elem_storage_type = *elem_type;

        cleanup_func = resolve_autodtor_func(elem_type);
        if (!cleanup_func)
        {
          if ((elem_type->t & VT_BTYPE) == VT_STRUCT)
            register_struct_cleanups(elem_type, sym);
          continue;
        }

        field_sym = sym_copy(sym, &local_stack);
        if ((field_sym->v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
          sym_link(field_sym, 0);
        field_sym->v = SYM_FIELD;
        field_sym->type = elem_storage_type;
        field_sym->c = sym->c + field->c + i * elem_size;

        cls = sym_push2(&all_cleanups, SYM_FIELD | ++cur_scope->cl.n, 0, 0);
        cls->cleanup_sym = field_sym;
        cls->cleanup_func = cleanup_func;
        cls->next = cur_scope->cl.s;
        cur_scope->cl.s = cls;
      }
      continue;
    }

    cleanup_func = resolve_autodtor_func(&field->type);
    if (!cleanup_func)
    {
      if ((field->type.t & VT_BTYPE) == VT_STRUCT)
        register_struct_cleanups(&field->type, sym);
      continue;
    }

    field_sym = sym_copy(sym, &local_stack);
    if ((field_sym->v & ~SYM_STRUCT) < SYM_FIRST_ANOM)
      sym_link(field_sym, 0);
    field_sym->v = SYM_FIELD;
    field_sym->type = field->type;
    field_sym->c = sym->c + field->c;

    cls = sym_push2(&all_cleanups, SYM_FIELD | ++cur_scope->cl.n, 0, 0);
    cls->cleanup_sym = field_sym;
    cls->cleanup_func = cleanup_func;
    cls->next = cur_scope->cl.s;
    cur_scope->cl.s = cls;
  }
}

static void call_lifecycle_constructor(CType *type, int r, int addr, Sym *sym,
                                       Sym *ctor_func)
{
  call_lifecycle_constructor_raw(type, r, addr, sym, ctor_func);
}

static void emit_lifecycle_constructor_selected(CType *type, int r, int addr,
                                                  int addr_r2, Sym *sym,
                                                  Sym *ctor_func,
                                                  TokenString **call_args,
                                                  int call_arg_count)
{
  Sym *func_type, *sa;
  int nb_args, ai, saved_delim = tok;
  call_lifecycle_constructor_members(type, r, addr);
  save_lvalues();
  vpushsym(&ctor_func->type, ctor_func);
  vset(type, r, addr);
  vtop->r2 = addr_r2;
  if (sym)
    vtop->sym = sym;
  mk_pointer(&vtop->type);
  gaddrof();

  func_type = ctor_func->type.ref;
  sa = func_type->next;
  gfunc_param_typed(func_type, sa);
  if (sa)
    sa = sa->next;
  nb_args = 1;

  for (ai = 0; ai < call_arg_count; ++ai)
  {
    emit_saved_arg_for_param(call_args[ai], sa);
    gfunc_param_typed_with_conversions(func_type, sa);
    nb_args++;
    if (sa)
      sa = sa->next;
  }
  while (sa)
  {
    emit_default_arg(func_type, sa);
    nb_args++;
    sa = sa->next;
  }
  if (tok == TOK_EOF)
  {
    next();
    unget_tok(saved_delim);
  }
  vcheck_cmp();
  gfunc_call(nb_args);
  drop_leaked_call_target(ctor_func);
}

static void call_lifecycle_constructor_saved_args_ex(CType *type, int r, int addr,
                                                  int addr_r2, Sym *sym,
                                                  Sym *ctor_func,
                                                  TokenString **call_args,
                                                  int call_arg_count,
                                                  int copy_initialization)
{
  /* Constructors address the unqualified object during its lifetime start. */
  CType constructing_type = *type;
  constructing_type.t &= ~(VT_CONSTANT | VT_VOLATILE);
  type = &constructing_type;
  CType call_arg_types[32];
  int saved_delim;

  if (copy_initialization == 2 && call_arg_count == 1
      && saved_arg_is_braced(call_args[0])
      && !class_has_initializer_list_constructor_candidate(type))
  {
    TokenString **elements;
    int count = split_saved_array_initializer(call_args[0], &elements);
    if (count > 32) cprime_error("too many constructor arguments");
    call_lifecycle_constructor_saved_args_ex(type, r, addr, addr_r2, sym,
                                             ctor_func, elements, count, 2);
    free_saved_array_elements(elements, count);
    return;
  }
  saved_delim = tok;
  infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
  if (tok == TOK_EOF)
  {
    next();
    unget_tok(saved_delim);
  }
  {
    Sym *overload = copy_initialization == 1
      ? resolve_implicit_constructor_func(type, call_arg_types, call_arg_count)
      : resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                          call_arg_types, call_arg_count);
    Sym *copy_constructor = NULL;
    int same_type_copy_init = 0;
    if (call_arg_count == 1)
    {
      CType arg_value_type = call_arg_types[0];
      if (is_reference_type(&arg_value_type))
        arg_value_type = *pointed_type(&arg_value_type);
      arg_value_type.t &= ~VT_RVALUE_REFERENCE;
      same_type_copy_init = is_compatible_unqualified_types(type,
                                                            &arg_value_type);
      if (same_type_copy_init)
      {
        copy_constructor = resolve_copy_constructor_func(type, &call_arg_types[0]);
        if (copy_initialization && copy_constructor && copy_constructor->type.ref
            && copy_constructor->type.ref->f.func_cpp_explicit)
        {
          if (copy_initialization != 1 || !overload)
            cprime_error("explicit copy constructor is not eligible for copy-initialization");
          copy_constructor = NULL;
        }
        if (copy_constructor)
          overload = copy_constructor;
      }
    }
    if ((same_type_copy_init && !copy_constructor
         && (!copy_initialization || !overload))
        || (call_arg_count == 1 && overload
            && is_defaulted_lifecycle_constructor(type, overload)))
    {
      init_params p = {0};
      TokenString macro = *call_args[0];
      TokenString *saved_macro_stack = macro_stack;
      const int *saved_macro_ptr = macro_ptr;
      begin_macro(&macro, 0);
      next();
      expr_eq();
      while (macro_stack && macro_stack != saved_macro_stack)
        end_macro();
      macro_ptr = saved_macro_ptr;
      init_putv(&p, type, addr);
      if (tok == TOK_EOF)
      {
        next();
        unget_tok(saved_delim);
      }
      return;
    }
    if (call_arg_count == 1 && saved_arg_is_braced(call_args[0]))
    {
      Sym *initializer_list_ctor = resolve_initializer_list_constructor(type);
      if (initializer_list_ctor)
        overload = initializer_list_ctor;
    }
    if (!overload && copy_initialization == 1)
      cprime_error("no implicit constructor for copy-initialization");
    if (!overload)
      overload = resolve_member_field_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                        call_arg_types,
                                                        call_arg_count);
    if (overload)
      ctor_func = overload;
    else
    {
      overload = resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1,
                                                  call_arg_count);
      if (overload)
        ctor_func = overload;
      else
        cprime_error("no matching constructor for initialization of '%s' with %d arguments",get_tok_str(get_struct_type_name_tok(type),NULL),call_arg_count);
    }
  }
  if (!ctor_func || (ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref)
    cprime_error("constructor target is not declared as function");
  if (copy_initialization && ctor_func->type.ref->f.func_cpp_explicit)
    cprime_error("explicit constructor is not eligible for copy-initialization");


  emit_lifecycle_constructor_selected(type, r, addr, addr_r2, sym,
                                        ctor_func, call_args, call_arg_count);
}

static void call_lifecycle_constructor_saved_args(CType *type, int r, int addr,
                                                  int addr_r2, Sym *sym,
                                                  Sym *ctor_func,
                                                  TokenString **call_args,
                                                  int call_arg_count)
{
  call_lifecycle_constructor_saved_args_ex(type, r, addr, addr_r2, sym,
                                           ctor_func, call_args, call_arg_count, 0);
}


typedef struct CppConstructionDestination {
  CType type;
  int address, storage, available;
  int conditional_marker;
} CppConstructionDestination;

static CppConstructionDestination *cpp_construction_destination;

static int take_cpp_construction_destination(CType *type, int *address,
                                              int *storage)
{
  if (nocode_wanted || !cpp_construction_destination
      || !cpp_construction_destination->available
      || cpp_construction_destination->conditional_marker
      || !is_compatible_unqualified_types(
           type, &cpp_construction_destination->type))
    return 0;
  *address = cpp_construction_destination->address;
  *storage = cpp_construction_destination->storage;
  cpp_construction_destination->available = 0;
  return 1;
}

static void push_cpp_construction_lvalue(CType *type, int storage, int address,
                                          int offset)
{
  if ((storage & VT_VALMASK) != VT_LLOCAL) {
    vset(type, storage, address + offset);
    return;
  }
  vset(type, storage, address);
  if (offset) {
    mk_pointer(&vtop->type); gaddrof();
    vtop->type = char_pointer_type;
    vpushi(offset); gen_op('+');
    vtop->type = *type; mk_pointer(&vtop->type);
    indir();
  }
}


static int try_parse_cpp_functional_constructor(int type_tok)
{
  Sym *struct_sym, *ctor_func;
  TokenString *call_args[32];
  CType call_arg_types[32];
  CType type;
  int size, align, addr, r2, call_arg_count;
  int direct_destination = 0, direct_address = 0;
  int storage = VT_LOCAL | VT_LVAL;

  struct_sym = struct_find(type_tok);
  if (struct_sym && (struct_sym->type.t & VT_BTYPE) == VT_STRUCT)
  {
    type.t = struct_sym->type.t;
    type.ref = struct_sym;
  }
  else
  {
    Sym *alias = sym_find(type_tok);
    if (!alias)
      alias = global_symbol_find(type_tok);
    if (!alias || !(alias->type.t & VT_TYPEDEF)
        || (alias->type.t & VT_BTYPE) != VT_STRUCT)
      return 0;
    type = alias->type;
    type.t &= ~VT_TYPEDEF;
  }

  if (tok == '{')
  {
    materialize_braced_temporary(&type);
    return 1;
  }

  if (tok != '(')
    return 0;

  if (take_cpp_construction_destination(&type, &direct_address, &storage))
  {
    /* Claim the final object's storage before parsing constructor arguments:
       a nested same-type argument is a separate object. */
    direct_destination = 1;
  }
  next();
  skip_line_markers();
  call_arg_count = count_saved_call_args(call_args, 32);
  if (call_arg_count == 1 && saved_arg_is_braced(call_args[0])
      && !class_has_initializer_list_constructor_candidate(&type))
  {
    TokenString macro = *call_args[0];
    TokenString *saved_macro_stack = macro_stack;
    const int *saved_macro_ptr = macro_ptr;
    CppConstructionDestination forwarded, *saved_destination = cpp_construction_destination;

    begin_macro(&macro, 0);
    next();
    if (direct_destination) {
      forwarded.type = type;
      forwarded.address = direct_address;
      forwarded.storage = storage;
      forwarded.available = 1;
      forwarded.conditional_marker = 0;
      cpp_construction_destination = &forwarded;
    }
    materialize_braced_temporary(&type);
    cpp_construction_destination = saved_destination;
    while (macro_stack && macro_stack != saved_macro_stack)
      end_macro();
    macro_ptr = saved_macro_ptr;
    if (tok == TOK_EOF)
      next();
    else
      skip(')');
    return 1;
  }
  infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
  if (call_arg_count == 1)
  {
    CType source_type = call_arg_types[0];
    if (is_reference_type(&source_type))
      source_type = *pointed_type(&source_type);
    source_type.t &= ~VT_RVALUE_REFERENCE;
    if (is_compatible_unqualified_types(&type, &source_type)
        && !resolve_copy_constructor_func(&type, &call_arg_types[0]))
    {
      SValue destination, source;
      /* Functional syntax also invokes an implicit copy/move constructor.
         Copy subobjects through their constructors, retaining the temporary
         destination on the value stack while the source is evaluated. */
      size = type_size(&type, &align);
      if (direct_destination) { addr = direct_address; r2 = VT_CONST; }
      else addr = get_temp_local_var(size, align, &r2);
      vset(&type, storage, addr);
      vtop->r2 = r2;
      emit_saved_arg_for_param(call_args[0], NULL);
      tok_str_free(call_args[0]);
      maybe_indir_reference();
      mk_pointer(&vtop->type);
      gaddrof();
      source = *vtop--;
      vpushv(vtop);
      mk_pointer(&vtop->type);
      gaddrof();
      destination = *vtop--;
      copy_construct_struct_memberwise_from_base_ptr(
        &type, &destination, &source, 0,
        (call_arg_types[0].t & VT_RVALUE_REFERENCE) != 0);
      initialize_virtual_tables_for_pointer(&type, &destination, 0);
      if (tok == TOK_EOF) next();
      else skip(')');
      if (!direct_destination) cpp_register_class_temporary(vtop);
      return 1;
    }
  }
  ctor_func = resolve_member_func_by_arg_types(&type, TOK_CONSTRUCTOR1,
                                               call_arg_types,
                                               call_arg_count);
  if (!ctor_func)
    ctor_func = resolve_member_field_func_by_arg_types(&type, TOK_CONSTRUCTOR1,
                                                       call_arg_types,
                                                       call_arg_count);
  if (!ctor_func)
    ctor_func = resolve_member_func_by_arg_count(&type, TOK_CONSTRUCTOR1,
                                                 call_arg_count);
  if (!ctor_func)
    ctor_func = resolve_lifecycle_func(&type, TOK_CONSTRUCTOR1);
  if (!ctor_func)
  {
    if (call_arg_count != 0)
      cprime_error("no matching constructor for '%s' with %d argument(s)",
                   get_tok_str(type_tok, NULL), call_arg_count);
    size = type_size(&type, &align);
    if (direct_destination) { addr = direct_address; r2 = VT_CONST; }
    else addr = get_temp_local_var(size, align, &r2);
    vset(&type, storage, addr);
    vtop->r2 = r2;
    call_lifecycle_constructor_members(&type, storage, addr);
    if (tok == TOK_EOF)
      next();
    else
      skip(')');
    if (!direct_destination) cpp_register_class_temporary(vtop);
    return 1;
  }

  size = type_size(&type, &align);
  if (direct_destination) { addr = direct_address; r2 = VT_CONST; }
  else addr = get_temp_local_var(size, align, &r2);
  vset(&type, storage, addr);
  vtop->r2 = r2;
  call_lifecycle_constructor_saved_args(&type, storage, addr,
                                        r2, NULL, ctor_func, call_args,
                                        call_arg_count);
  for (int ai = 0; ai < call_arg_count; ++ai) tok_str_free(call_args[ai]);
  if (tok == TOK_EOF)
    next();
  else
    skip(')');

  if (!direct_destination) cpp_register_class_temporary(vtop);
  return 1;
}

static int try_parse_cpp_functional_type_cast(int type_tok)
{
  Sym *type_sym;
  CType cast_type;

  if (tok != '(')
    return 0;

  type_sym = struct_find(type_tok);
  if (type_sym && IS_ENUM(type_sym->type.t))
  {
    cast_type = type_sym->type;
    /* An explicit enum conversion still has the enumeration type.  Its
       underlying integer representation must not replace that identity:
       overload resolution can bind the result to an enum reference. */
    cast_type.t &= ~VT_STORAGE;
    cast_type.ref = type_sym;
  }
  else
  {
    type_sym = sym_find(type_tok);
    if (!type_sym)
      type_sym = global_symbol_find(type_tok);
    if (!type_sym || !(type_sym->type.t & VT_TYPEDEF)
        || (type_sym->type.t & VT_BTYPE) == VT_STRUCT)
      return 0;
    cast_type = type_sym->type;
    cast_type.t &= ~VT_TYPEDEF;
  }

  next();
  if (tok == ')')
  {
    next();
    vpushi(0);
  }
  else
  {
    expr_eq();
    skip(')');
  }

  if (is_reference_type(&cast_type))
  {
    Sym parameter, function;
    memset(&parameter, 0, sizeof(parameter));
    memset(&function, 0, sizeof(function));
    parameter.type = cast_type;
    function.f.func_type = FUNC_NEW;
    if ((vtop->type.t & VT_BTYPE) == VT_STRUCT)
      try_call_cpp_conversion_operator(&cast_type, 1);
    gfunc_param_typed(&function, &parameter);
    vtop->type = cast_type;
    maybe_indir_call_result();
    return 1;
  }
  if ((cast_type.t & VT_BTYPE) == VT_BOOL
      && (vtop->type.t & VT_BTYPE) == VT_STRUCT)
    try_call_cpp_bool_conversion_operator();
  gen_cast(&cast_type);
  return 1;
}

static void replay_saved_placement_expr(TokenString *expr)
{
  TokenString macro = *expr;
  TokenString *parent = macro_stack;
  const int *parent_ptr = macro_ptr;

  /* Saved placement expressions remain owned by their caller.  Replay a
     non-owning stack view: using the heap TokenString itself as an owning
     macro lets end_macro() free it, and a later explicit cleanup then double
     frees the stream (eventually reclaiming live replay state). */
  begin_macro(&macro, 0);
  next();
  expr_eq();
  while (macro_stack && macro_stack != parent)
    end_macro();
  macro_ptr = parent_ptr;
}

static void free_saved_placement_args(TokenString *placement,
                                      TokenString **call_args,
                                      int call_arg_count)
{
  int i;
  for (i = 0; i < call_arg_count; ++i)
    tok_str_free(call_args[i]);
  tok_str_free(placement);
}

static void call_placement_constructor(CType *type, TokenString *placement,
                                       TokenString **call_args,
                                       int call_arg_count)
{
  Sym *ctor_func, *func_type, *sa;
  CType call_arg_types[32], ptr_type;
  int nb_args, ai, saved_tok, use_implicit_memberwise_copy = 0;
  CValue saved_tokc;

  if (call_arg_count == 1 && saved_arg_is_braced(call_args[0])
      && type->ref && !type->ref->a.lifecycle_ctor)
  {
    emit_placement_aggregate_initializer(type, placement, call_args[0]);
    free_saved_placement_args(placement, call_args, call_arg_count);
    return;
  }

  if (call_arg_count == 1
      && saved_initializer_is_direct_constructor(call_args[0], type))
  {
    CppConstructionDestination destination, *previous;
    int slot;
    saved_tok = tok; saved_tokc = tokc;
    replay_saved_placement_expr(placement);
    ptr_type = *type;
    mk_pointer(&ptr_type);
    gen_cast(&ptr_type);
    /* Evaluate the placement address once and keep both new's pointer result
       and an indirect destination for constructor/return-value forwarding. */
    /* The destination outlives argument conversion temporaries and is held
       outside the value stack, so reserve a local slot that their allocator
       cannot recycle. */
    loc = (loc - PTR_SIZE) & -PTR_SIZE;
    slot = loc;
    vdup();
    vset(&ptr_type, VT_LOCAL | VT_LVAL, slot);
    vswap(); vstore(); vpop();
    destination.type = *type;
    destination.address = slot;
    destination.storage = VT_LLOCAL | VT_LVAL;
    destination.available = 1;
    destination.conditional_marker = cpp_prepare_conditional_destination(call_args[0]);
    previous = cpp_construction_destination;
    cpp_construction_destination = &destination;
    replay_saved_placement_expr(call_args[0]);
    cpp_construction_destination = previous;
    if (destination.available)
    {
      vset(type, VT_LLOCAL | VT_LVAL, slot);
      vswap(); vstore();
    }
    vpop();
    tok = saved_tok; tokc = saved_tokc;
    free_saved_placement_args(placement, call_args, call_arg_count);
    return;
  }

  if (getenv("CPC_TRACE_PLACEMENT"))
  {
    fprintf(stderr, "CPC_PLACEMENT type=%s argc=%d current=%s stack=%p args=",
            get_tok_str(get_struct_type_name_tok(type), NULL), call_arg_count,
            get_tok_str(tok, &tokc), (void *)macro_stack);
    if (macro_stack)
      fprintf(stderr, "{macro alloc=%d len=%d off=%d} ", macro_stack->alloc,
              macro_stack->len, (int)(macro_ptr - macro_stack->str));
    for (ai = 0; ai < call_arg_count; ++ai)
    {
      const int *tp = call_args[ai]->str;
      const int *te = tp + call_args[ai]->len;
      CValue tv;
      int tt, tc = 0;
      fprintf(stderr, "%s[", ai ? "," : "");
      while (tp < te && tc++ < 24)
      {
        TOK_GET(&tt, &tp, &tv);
        if (tt == TOK_EOF || !tt)
          break;
        fprintf(stderr, "%s%s", tc > 1 ? " " : "", get_tok_str(tt, &tv));
      }
      fprintf(stderr, "]");
    }
    fprintf(stderr, "\n");
  }
  infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
  if (getenv("CPC_TRACE_PLACEMENT"))
    fprintf(stderr, "CPC_PLACEMENT after-infer current=%s stack=%p len=%d\n",
            get_tok_str(tok, &tokc), (void *)macro_stack,
            macro_stack ? macro_stack->len : -1);
  ctor_func = call_arg_count == 1 && saved_arg_is_braced(call_args[0])
                ? resolve_initializer_list_constructor(type) : NULL;
  if (!ctor_func)
    ctor_func = resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                 call_arg_types,
                                                 call_arg_count);
  if (call_arg_count == 1)
  {
    CType source = call_arg_types[0];
    Sym *copy_constructor;
    if (is_reference_type(&source)) source = *pointed_type(&source);
    source.t &= ~VT_RVALUE_REFERENCE;
    if (is_compatible_unqualified_types(type, &source))
    {
      copy_constructor = resolve_copy_constructor_func(type, &call_arg_types[0]);
      ctor_func = copy_constructor;
      use_implicit_memberwise_copy = !copy_constructor;
    }
  }
  if (!ctor_func)
    ctor_func = resolve_member_field_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                       call_arg_types,
                                                       call_arg_count);
  if (!ctor_func && call_arg_count == 1)
  {
    CType source_value_type = call_arg_types[0];
    source_value_type.t &= ~VT_RVALUE_REFERENCE;
    if (is_reference_type(&source_value_type))
      source_value_type = *pointed_type(&source_value_type);
    use_implicit_memberwise_copy =
      (source_value_type.t & VT_BTYPE) == VT_STRUCT
      && is_compatible_unqualified_types(type, &source_value_type);
  }
  /* A class retains its implicit copy constructor when it also has unrelated
     one-argument constructors.  If overload discovery found no authored
     self-type constructor, do not let the argument-count fallback select a
     scalar/converting constructor for a same-type placement copy.  The null
     target below is the existing memberwise implicit-copy path. */
  if (!ctor_func && !use_implicit_memberwise_copy)
    ctor_func = resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1,
                                                call_arg_count);
  if (!ctor_func && !use_implicit_memberwise_copy)
    ctor_func = call_arg_count == 0 ? resolve_autoctor_func(type)
                                   : resolve_lifecycle_func(type, TOK_CONSTRUCTOR1);
  if (getenv("CPC_TRACE_PLACEMENT"))
  {
    Sym *trace_param = ctor_func && ctor_func->type.ref
                       ? ctor_func->type.ref->next : NULL;
    fprintf(stderr, "CPC_PLACEMENT resolved=%s params=",
            ctor_func ? get_tok_str(ctor_func->v, NULL) : "<none>");
    for (; trace_param; trace_param = trace_param->next)
      fprintf(stderr, "%s,", get_tok_str(trace_param->v & ~SYM_FIELD, NULL));
    fprintf(stderr, " current=%s stack=%p len=%d\n", get_tok_str(tok, &tokc),
            (void *)macro_stack, macro_stack ? macro_stack->len : -1);
  }
  if (ctor_func
      && ((ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref))
    cprime_error("placement constructor target is not a function");

  /* Defaulted copy/move constructors have no user-authored body to call.
     Route same-type placement construction through the memberwise path
     below instead of emitting a call to their declaration stub. */
  if (ctor_func && call_arg_count == 1
      && is_defaulted_lifecycle_constructor(type, ctor_func))
    ctor_func = NULL;

  saved_tok = tok;
  saved_tokc = tokc;
  replay_saved_placement_expr(placement);
  tok = saved_tok;
  tokc = saved_tokc;

  ptr_type = *type;
  mk_pointer(&ptr_type);
  gen_cast(&ptr_type);
  /* Keep the value returned by new stable while individual subobjects are
     addressed and constructed through their own receiver pointers. */
  save_regs(0);

  if (!ctor_func)
  {
    SValue base_ptr;
    if (call_arg_count == 1)
    {
      CType source_value_type = call_arg_types[0];
      SValue source, source_ptr;

      source_value_type.t &= ~VT_RVALUE_REFERENCE;
      if (is_reference_type(&source_value_type))
        source_value_type = *pointed_type(&source_value_type);
      if ((source_value_type.t & VT_BTYPE) == VT_STRUCT
          && is_compatible_unqualified_types(type, &source_value_type))
      {
        saved_tok = tok;
        saved_tokc = tokc;
        replay_saved_placement_expr(call_args[0]);
        tok = saved_tok;
        tokc = saved_tokc;

        source = *vtop;
        vtop--;
        /* Keep the original placement pointer live on the value stack while
           replaying the source expression.  Calls in that expression can
           otherwise invalidate a register-only destination address. */
        base_ptr = *vtop;
        vpushv(&source);
        mk_pointer(&vtop->type);
        gaddrof();
        source_ptr = *vtop;
        vtop--;
        copy_construct_struct_memberwise_from_base_ptr(
          type, &base_ptr, &source_ptr, 0,
          (call_arg_types[0].t & VT_RVALUE_REFERENCE) != 0);
        initialize_virtual_tables_for_pointer(type, &base_ptr, 0);
        free_saved_placement_args(placement, call_args, call_arg_count);
        return;
      }
    }
    if (call_arg_count)
      cprime_error("no matching constructor for placement new of '%s' with %d arguments",
                   get_tok_str(get_struct_type_name_tok(type), NULL), call_arg_count);
    base_ptr = *vtop;
    call_lifecycle_constructor_members_base_ptr(type, &base_ptr, 0);
    initialize_virtual_tables_for_pointer(type, &base_ptr, 0);
    free_saved_placement_args(placement, call_args, call_arg_count);
    return;
  }

  save_lvalues();
  vdup();
  vpushsym(&ctor_func->type, ctor_func);
  vswap();

  func_type = ctor_func->type.ref;
  sa = func_type->next;
  gfunc_param_typed(func_type, sa);
  if (sa)
    sa = sa->next;
  nb_args = 1;

  for (ai = 0; ai < call_arg_count; ++ai)
  {
    TokenString *trace_parent = macro_stack;
    if (getenv("CPC_TRACE_PLACEMENT"))
    {
      const int *tp = call_args[ai]->str;
      const int *te = tp + call_args[ai]->len;
      CValue tv;
      int tt, tc = 0;
      fprintf(stderr, "CPC_PLACEMENT emit%d=", ai);
      while (tp < te && tc++ < 24)
      {
        TOK_GET(&tt, &tp, &tv);
        if (tt == TOK_EOF || !tt)
          break;
        fprintf(stderr, "%s%s", tc > 1 ? " " : "", get_tok_str(tt, &tv));
      }
      fprintf(stderr, "\n");
    }
    saved_tok = tok;
    saved_tokc = tokc;
    if (getenv("CPC_TRACE_PLACEMENT"))
      fprintf(stderr, "CPC_PLACEMENT emit-begin%d\n", ai);
    /* Placement owns these clauses and releases them after replay. */
    if (saved_arg_is_braced(call_args[ai]) && sa
        && ((sa->type.t & VT_BTYPE) == VT_STRUCT
            || (is_reference_type(&sa->type)
                && (pointed_type(&sa->type)->t & VT_BTYPE) == VT_STRUCT)))
      emit_saved_arg_for_param(call_args[ai], sa);
    else
      replay_saved_placement_expr(call_args[ai]);
    if (getenv("CPC_TRACE_PLACEMENT"))
      fprintf(stderr, "CPC_PLACEMENT emit-done%d parent=%p current=%p\n",
              ai, (void *)trace_parent, (void *)macro_stack);
    tok = saved_tok;
    tokc = saved_tokc;
    if (getenv("CPC_TRACE_PLACEMENT"))
      fprintf(stderr, "CPC_PLACEMENT param-begin%d\n", ai);
    gfunc_param_typed_with_conversions(func_type, sa);
    if (getenv("CPC_TRACE_PLACEMENT"))
      fprintf(stderr, "CPC_PLACEMENT param-done%d\n", ai);
    nb_args++;
    if (sa)
      sa = sa->next;
  }
  while (sa)
  {
    emit_default_arg(func_type, sa);
    nb_args++;
    sa = sa->next;
  }

  vcheck_cmp();
  if (getenv("CPC_TRACE_PLACEMENT"))
    fprintf(stderr, "CPC_PLACEMENT call-begin\n");
  gfunc_call(nb_args);
  if (getenv("CPC_TRACE_PLACEMENT"))
  {
    const int *tp = macro_ptr;
    const int *te = macro_stack ? macro_stack->str + macro_stack->len : tp;
    CValue tv;
    int tt, tc = 0;
    fprintf(stderr, "CPC_PLACEMENT call-done remaining=");
    if (macro_stack)
      fprintf(stderr, "{macro alloc=%d len=%d off=%d} ", macro_stack->alloc,
              macro_stack->len, (int)(macro_ptr - macro_stack->str));
    while (tp < te && tc++ < 20)
    {
      TOK_GET(&tt, &tp, &tv);
      if (tt == TOK_EOF || !tt)
        break;
      fprintf(stderr, "%s%s", tc > 1 ? " " : "", get_tok_str(tt, &tv));
    }
    fprintf(stderr, "\n");
  }
  drop_leaked_call_target(ctor_func);
  if ((vtop->type.t & VT_BTYPE) == VT_VOID)
    vpop();
  /* The constructor initializes its own vptrs. In particular, a native
     constructor installs the native ABI's vtables, which callers must retain. */
  free_saved_placement_args(placement, call_args, call_arg_count);
}

static Sym *cpp_allocation_function(void)
{
  int name_tok = tok_alloc_const("malloc");
  Sym *function = global_symbol_find(name_tok);
  if (!function)
  {
    CType result = { VT_VOID, NULL };
    CType function_type;
    Sym *prototype, *argument;
    mk_pointer(&result);
    prototype = sym_push2(&global_stack, SYM_FIELD, result.t, 0);
    prototype->type.ref = result.ref;
    prototype->f.func_call = FUNC_CDECL;
    prototype->f.func_type = FUNC_NEW;
    argument = sym_push2(&global_stack, SYM_FIELD, VT_SIZE_T, 0);
    prototype->next = argument;
    function_type.t = VT_FUNC;
    function_type.ref = prototype;
    function = external_global_sym(name_tok, &function_type);
  }
  return function;
}

static int try_parse_cpp_placement_new_after_name(void)
{
  CType type;
  AttributeDef ad;
  TokenString *placement, *call_args[32];
  int call_arg_count = 0, saved_tok;
  CValue saved_tokc;

  if (tok != '(')
  {
    CType ptr_type;
    Sym *malloc_sym;
    int align, size;
    CValue size_value;

    memset(&ad, 0, sizeof(ad));
    if (!parse_btype(&type, &ad, 0))
      return 0;
    while (tok == '*')
    {
      mk_pointer(&type);
      next();
      while (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3
             || tok == TOK_VOLATILE1 || tok == TOK_VOLATILE2 || tok == TOK_VOLATILE3)
      {
        type.t |= (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3)
                    ? VT_CONSTANT : VT_VOLATILE;
        next();
      }
    }
    size = type_size(&type, &align);
    if (size < 0)
      cprime_error("new requires a complete element type");
    /* Built-in allocation cannot depend on a source-level malloc prototype.
       In particular, an implicit int return would truncate Win64 pointers. */
    malloc_sym = cpp_allocation_function();
    if (tok == '[')
    {
      cpp_new_array(&type, malloc_sym, size, align);
      return 1;
    }

    /* Lower scalar allocation to the same path as placement new so class
       constructors, constructor arguments, and scalar initialization share
       one implementation. */
    placement = tok_str_alloc();
    tok_str_add(placement, tok_alloc_const("malloc"));
    tok_str_add(placement, '(');
    size_value.i = size;
    tok_str_add2(placement, TOK_CINT, &size_value);
    tok_str_add(placement, ')');
    tok_str_add(placement, TOK_EOF);
  }
  else
  {
    next();
    skip_or_save_block(&placement);
    skip(')');

    memset(&ad, 0, sizeof(ad));
    if (!parse_btype(&type, &ad, 0))
      cprime_error("placement new requires a type before '%s'", get_tok_str(tok, &tokc));
  }

  if (tok == '(')
  {
    next();
    call_arg_count = count_saved_call_args(call_args, 32);
    skip(')');
  }
  if ((type.t & VT_BTYPE) != VT_STRUCT)
  {
    CType ptr_type = type;

    if (call_arg_count > 1)
      cprime_error("scalar placement new accepts at most one argument");
    saved_tok = tok;
    saved_tokc = tokc;
    replay_saved_placement_expr(placement);
    tok = saved_tok;
    tokc = saved_tokc;
    mk_pointer(&ptr_type);
    gen_cast(&ptr_type);
    vdup();
    indir();
    if (call_arg_count)
    {
      replay_saved_placement_expr(call_args[0]);
      tok = saved_tok;
      tokc = saved_tokc;
    }
    else
      vpushi(0);
    vstore();
    vpop();
    free_saved_placement_args(placement, call_args, call_arg_count);
    return 1;
  }
  saved_tok = tok;
  saved_tokc = tokc;
  call_placement_constructor(&type, placement, call_args, call_arg_count);
  tok = saved_tok;
  tokc = saved_tokc;
  return 1;
}

static Sym *resolve_member_func(CType *type, int method_tok)
{
  int struct_tok, owner_tok, mangled_tok;
  Sym *s, *field;
  CType struct_type, lowered_type;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  if (is_lifecycle_member_tok(method_tok))
    return resolve_lifecycle_func(type, method_tok);
  owner_tok = struct_tok;
  if (!is_lifecycle_member_tok(method_tok))
    instantiate_template_member_for_call(type, method_tok, NULL, -1);
  mangled_tok = make_member_func_tok(struct_tok, method_tok);
  s = sym_find(mangled_tok);
  if (!s)
    s = global_symbol_find(mangled_tok);
  if (s && (s->type.t & VT_BTYPE) != VT_FUNC
      && !((s->type.t & VT_BTYPE) == VT_PTR
           && ((pointed_type(&s->type)->t & VT_BTYPE) == VT_FUNC)))
    s = NULL;
  if (!s)
  {
    /* Fallback: recover from field-only member declarations by lowering here. */
    field = find_field_try_with_owner(type, method_tok, &struct_tok,
                                      &owner_tok);
    if (!field)
      field = find_field_try_with_owner(type, method_tok | SYM_FIELD,
                                        &struct_tok, &owner_tok);
    if (field && (field->type.t & VT_BTYPE) == VT_FUNC)
    {
      if (!make_class_type_from_tok(&struct_type, owner_tok))
        struct_type = *type;
      mangled_tok = make_member_func_tok(owner_tok, method_tok);
      lowered_type = make_lowered_member_func_type(&struct_type, &field->type);
      s = external_global_sym(mangled_tok, &lowered_type);
    }
  }
  else
  {
    /* Prefer non-void lowered signature from class member declaration. */
    field = find_field_try_with_owner(type, method_tok, &struct_tok,
                                      &owner_tok);
    if (!field)
      field = find_field_try_with_owner(type, method_tok | SYM_FIELD,
                                        &struct_tok, &owner_tok);
    if (field && (field->type.t & VT_BTYPE) == VT_FUNC
        && s->type.ref
        && (s->type.ref->type.t & VT_BTYPE) == VT_VOID
        && (field->type.ref->type.t & VT_BTYPE) != VT_VOID)
    {
      if (!make_class_type_from_tok(&struct_type, owner_tok))
        struct_type = *type;
      mangled_tok = make_member_func_tok(owner_tok, method_tok);
      lowered_type = make_lowered_member_func_type(&struct_type, &field->type);
      s = external_global_sym(mangled_tok, &lowered_type);
    }
  }
  if (!s)
    return NULL;
  if ((s->type.t & VT_BTYPE) != VT_FUNC)
    cprime_error("member function target '%s' is not declared as function",
              get_tok_str(mangled_tok, NULL));
  return s;
}

static int member_receiver_is_const(CType *type)
{
  return (type->t & VT_CONSTANT) != 0;
}

static int member_overload_receiver_ok(MemberFuncOverload *o, int receiver_const, CType *receiver)
{
  int ref = o->func_type.ref ? o->func_type.ref->f.func_ref_qualifier : 0;
  int rvalue = (receiver->t & VT_RVALUE_REFERENCE) != 0;
  if (ref == 2 && !rvalue) return 0;
  if (ref == 1 && rvalue && !o->is_const) return 0;
  return o->is_const || !receiver_const;
}

static void member_overload_note_match(Sym **match, Sym **const_match,
                                       Sym *s, int is_const)
{
  Sym **slot = is_const ? const_match : match;
  if (!*slot)
    *slot = s;
}

static void member_overload_note_ranked_match(Sym **match, Sym **const_match,
                                              int *best_omitted_defaults,
                                              Sym *s, int is_const,
                                              int omitted_defaults)
{
  Sym **slot;

  if (omitted_defaults > *best_omitted_defaults)
    return;
  if (omitted_defaults < *best_omitted_defaults)
  {
    *match = NULL;
    *const_match = NULL;
    *best_omitted_defaults = omitted_defaults;
  }
  slot = is_const ? const_match : match;
  if (!*slot)
    *slot = s;
}

static Sym *member_overload_pick_match(Sym *match, Sym *const_match,
                                       int receiver_const)
{
  Sym *selected = receiver_const ? const_match : match ? match : const_match;
  if (selected) deduce_pending_member_return(selected->v);
  return selected;
}

static Sym *resolve_member_func_by_arg_count(CType *type, int method_tok,
                                             int explicit_arg_count)
{
  int struct_tok;
  MemberFuncOverload *o;
  Sym *match = NULL;
  Sym *const_match = NULL;
  int receiver_const = is_lifecycle_member_tok(method_tok)
                         ? 0 : member_receiver_is_const(type);
  int best_omitted_defaults = 0x7fffffff;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  instantiate_template_member_for_call(type, method_tok, NULL,
                                       explicit_arg_count);
  for (o = member_func_candidates(struct_tok, method_tok); o;)
  {
    MemberFuncOverload *next_o = o->bucket_next;
    Sym *s;
    if (profile_scans_enabled) ++profile_other_overload_scans[0];
    if (getenv("CPC_TRACE_CLBITREF")
        && struct_tok >= TOK_UIDENT
        && strstr(get_tok_str(struct_tok, NULL), "clBitRef")
        && method_tok == tok_alloc_const("operator="))
      fprintf(stderr, "CPC_CLBITREF_CAND cand=%s argc=%d min=%d explicit=%d\n",
              get_tok_str(o->mangled_tok, NULL), explicit_arg_count,
              o->min_arg_count, o->explicit_arg_count);
    if (o->struct_tok != struct_tok || o->method_tok != method_tok
        || explicit_arg_count < o->min_arg_count
        || explicit_arg_count > o->explicit_arg_count)
      goto next_overload;
    if (!member_overload_receiver_ok(o, receiver_const, type))
    {
      if (getenv("CPC_TRACE_CLBITREF")
          && struct_tok >= TOK_UIDENT
          && strstr(get_tok_str(struct_tok, NULL), "clBitRef")
          && method_tok == tok_alloc_const("operator="))
        fprintf(stderr, "CPC_CLBITREF_SKIP_RECEIVER cand=%s recv_const=%d o_const=%d type_t=%04x\n",
                get_tok_str(o->mangled_tok, NULL), receiver_const,
                o->is_const, type->t);
      goto next_overload;
    }
    s = global_symbol_find(o->mangled_tok);
    if (!s)
      s = sym_find(o->mangled_tok);
    if (!s)
      goto next_overload;
    use_overload_func_type(s, &o->func_type);
    if (method_tok == TOK_CONSTRUCTOR1
        && o->explicit_arg_count - explicit_arg_count == best_omitted_defaults)
    {
      Sym **slot = o->is_const ? &const_match : &match;
      *slot = s;
    }
    else
      member_overload_note_ranked_match(&match, &const_match,
                                        &best_omitted_defaults, s,
                                        o->is_const,
                                        o->explicit_arg_count - explicit_arg_count);
next_overload:
    o = next_o;
  }
  match = member_overload_pick_match(match, const_match, receiver_const);
  if (match && getenv("CPC_DUMP_MEMBER_RESOLVE")
      && (method_tok >= TOK_UIDENT || method_tok == TOK_CONSTRUCTOR1))
  {
    const char *method_name = get_tok_str(method_tok, NULL);
    if (method_tok == TOK_CONSTRUCTOR1
        || !strncmp(method_name, "operator", 8) || !strcmp(method_name, "Pack"))
      fprintf(stderr, "CPC_MEMBER_RESOLVE recv=%s method=%s argc=%d -> %s\n",
              get_tok_str(struct_tok, NULL), method_name,
              explicit_arg_count, get_tok_str(match->v, NULL));
  }
  if (match)
  {
    instantiate_template_member_body_for_func_tok(type, method_tok, match->v);
    return match;
  }
  /* Constructors/destructors belong to their declaring class. Implicit
     lifecycle work visits every base; it cannot select one inherited body. */
  if (is_lifecycle_member_tok(method_tok)) return NULL;
  {
    ClassBaseInfo *info;
    for (info = class_base_infos; info; info = info->next)
    {
      CType base_type;
      Sym *base_match;

      if (info->class_tok != struct_tok)
        continue;
      if (!make_class_type_from_tok(&base_type, info->base_tok))
        continue;
      base_type.t |= type->t & VT_CONSTANT;
      base_match = resolve_member_func_by_arg_count(&base_type, method_tok,
                                                    explicit_arg_count);
      if (base_match)
        return base_match;
    }
  }
  return explicit_arg_count < 0 ? resolve_member_func(type, method_tok) : NULL;
}

static int member_overload_exists_for_call(CType *type, int method_tok,
                                           int explicit_arg_count)
{
  int struct_tok;
  MemberFuncOverload *o;
  ClassBaseInfo *info;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return 0;
  for (o = member_func_candidates(struct_tok, method_tok);
       o; o = o->bucket_next)
  {
    if (profile_scans_enabled) ++profile_other_overload_scans[1];
    if (o->struct_tok == struct_tok && o->method_tok == method_tok
        && explicit_arg_count >= o->min_arg_count
        && explicit_arg_count <= o->explicit_arg_count)
      return 1;
  }
  if (is_lifecycle_member_tok(method_tok)) return 0;
  for (info = class_base_infos; info; info = info->next)
  {
    CType base_type;

    if (info->class_tok != struct_tok)
      continue;
    if (!make_class_type_from_tok(&base_type, info->base_tok))
      continue;
    base_type.t |= type->t & VT_CONSTANT;
    if (member_overload_exists_for_call(&base_type, method_tok,
                                        explicit_arg_count))
      return 1;
  }
  return 0;
}

static int member_func_owner_for_call(CType *type, int method_tok,
                                      int explicit_arg_count)
{
  int struct_tok;
  MemberFuncOverload *o;
  ClassBaseInfo *info;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return 0;
  for (o = member_func_candidates(struct_tok, method_tok);
       o; o = o->bucket_next)
  {
    if (profile_scans_enabled) ++profile_other_overload_scans[2];
    if (o->struct_tok == struct_tok && o->method_tok == method_tok
        && explicit_arg_count >= o->min_arg_count
        && explicit_arg_count <= o->explicit_arg_count)
      return struct_tok;
  }
  if (is_lifecycle_member_tok(method_tok)) return 0;
  for (info = class_base_infos; info; info = info->next)
  {
    CType base_type;
    int owner_tok;

    if (info->class_tok != struct_tok)
      continue;
    if (!make_class_type_from_tok(&base_type, info->base_tok))
      continue;
    base_type.t |= type->t & VT_CONSTANT;
    owner_tok = member_func_owner_for_call(&base_type, method_tok,
                                           explicit_arg_count);
    if (owner_tok)
      return owner_tok;
  }
  return 0;
}

static int member_overload_count_for_call(CType *type, int method_tok,
                                          int explicit_arg_count)
{
  int struct_tok, count = 0;
  MemberFuncOverload *o;
  ClassBaseInfo *info;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return 0;
  for (o = member_func_candidates(struct_tok, method_tok);
       o; o = o->bucket_next)
  {
    if (profile_scans_enabled) ++profile_other_overload_scans[3];
    if (o->struct_tok == struct_tok && o->method_tok == method_tok
        && explicit_arg_count >= o->min_arg_count
        && explicit_arg_count <= o->explicit_arg_count)
      ++count;
  }
  if (is_lifecycle_member_tok(method_tok)) return count;
  for (info = class_base_infos; info; info = info->next)
  {
    CType base_type;

    if (info->class_tok != struct_tok)
      continue;
    if (!make_class_type_from_tok(&base_type, info->base_tok))
      continue;
    base_type.t |= type->t & VT_CONSTANT;
    count += member_overload_count_for_call(&base_type, method_tok,
                                            explicit_arg_count);
  }
  return count;
}

static int type_is_std_initializer_list(CType *type)
{
  int struct_tok;

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
    return 0;
  struct_tok = get_struct_type_name_tok(type);
  return struct_tok >= TOK_UIDENT
         && strstr(get_tok_str(struct_tok, NULL), "initializer_list") != NULL;
}

static int class_has_single_arg_constructor_for(CType *class_type,
                                                CType *arg_type)
{
  CType arg_value_type, decayed_arg_type;
  Sym *class_sym, *field, *arg, *param_type;
  int class_tok, result = 0, arg_key_tok = 0, arg_key_btype;
  MemberFuncOverload *o;
  int i;
  static int ctor_match_depth;
  static struct
  {
    int class_tok;
    int arg_tok;
    int arg_btype;
  } ctor_match_stack[64];

  if ((class_type->t & VT_BTYPE) != VT_STRUCT)
    return 0;
  arg_value_type = *arg_type;
  arg_value_type.t &= ~VT_RVALUE_REFERENCE;
  if ((arg_value_type.t & VT_BTYPE) == VT_STRUCT
      && is_compatible_unqualified_types(class_type, &arg_value_type))
    return 0;
  class_tok = get_struct_type_name_tok(class_type);
  if (!class_tok)
    return 0;
  arg_key_btype = arg_value_type.t & (VT_BTYPE | VT_ARRAY | VT_VLA);
  if (is_reference_type(&arg_value_type))
    arg_key_tok = get_struct_type_name_tok(pointed_type(&arg_value_type));
  else if ((arg_value_type.t & VT_BTYPE) == VT_STRUCT)
    arg_key_tok = get_struct_type_name_tok(&arg_value_type);
  else if (((arg_value_type.t & VT_BTYPE) == VT_PTR
            || (arg_value_type.t & VT_ARRAY))
           && arg_value_type.ref)
  {
    CType pointed = *pointed_type(&arg_value_type);
    arg_key_btype |= (pointed.t & VT_BTYPE) << 8;
    arg_key_tok = get_struct_type_name_tok(&pointed);
  }
  if (ctor_match_depth >= (int)(sizeof ctor_match_stack / sizeof ctor_match_stack[0]))
    return 0;
  for (i = 0; i < ctor_match_depth; ++i)
    if (ctor_match_stack[i].class_tok == class_tok
        && ctor_match_stack[i].arg_tok == arg_key_tok
        && ctor_match_stack[i].arg_btype == arg_key_btype)
      return 0;
  ctor_match_stack[ctor_match_depth].class_tok = class_tok;
  ctor_match_stack[ctor_match_depth].arg_tok = arg_key_tok;
  ctor_match_stack[ctor_match_depth].arg_btype = arg_key_btype;
  ctor_match_depth++;

  for (o = member_func_candidates(class_tok, TOK_CONSTRUCTOR1);
       o; o = o->bucket_next)
  {
    Sym *s;
    if (profile_scans_enabled) ++profile_other_overload_scans[4];
    if (o->struct_tok != class_tok || o->method_tok != TOK_CONSTRUCTOR1
        || o->min_arg_count > 1 || o->explicit_arg_count < 1)
      continue;
    s = global_symbol_find(o->mangled_tok);
    if (!s)
      s = sym_find(o->mangled_tok);
    if (!s)
      continue;
    use_overload_func_type(s, &o->func_type);
    if (s->type.ref && s->type.ref->f.func_cpp_explicit)
      continue;
    param_type = s->type.ref ? s->type.ref->next : NULL;
    if (param_type)
      param_type = param_type->next; /* skip implicit this */
    if (param_type)
    {
      CType param_value_type = param_type->type;
      if (is_reference_type(&param_value_type))
        param_value_type = *pointed_type(&param_value_type);
      if ((param_value_type.t & VT_BTYPE) == VT_STRUCT
          && is_compatible_unqualified_types(class_type, &param_value_type))
        continue;
    }
    if (member_func_arg_match_rank(s, &arg_value_type, 1) >= 0)
    {
      result = 1;
      goto done;
    }
  }
  class_sym = struct_find(class_tok);
  if (!class_sym)
    goto done;
  for (field = class_sym->next; field; field = field->next)
  {
    if (((field->v & ~SYM_FIELD) != TOK_CONSTRUCTOR1
         && (field->v & ~SYM_FIELD) != class_tok)
        || (field->type.t & VT_BTYPE) != VT_FUNC
        || !field->type.ref)
      continue;
    if (field->type.ref->f.func_cpp_explicit)
      continue;
    arg = field->type.ref->next;
    if (!arg || arg->next)
      continue;
    {
      CType param_value_type = arg->type;
      if (is_reference_type(&param_value_type))
        param_value_type = *pointed_type(&param_value_type);
      if ((param_value_type.t & VT_BTYPE) == VT_STRUCT
          && is_compatible_unqualified_types(class_type, &param_value_type))
        continue;
    }
    if (call_arg_match_rank(&arg->type, &arg_value_type) >= 0)
    {
      result = 1;
      goto done;
    }
    if (arg_value_type.t & (VT_ARRAY | VT_VLA))
    {
      decayed_arg_type = arg_value_type;
      decayed_arg_type.t &= ~(VT_ARRAY | VT_VLA);
      if (call_arg_match_rank(&arg->type, &decayed_arg_type) >= 0)
      {
        result = 1;
        goto done;
      }
    }
  }
done:
  ctor_match_depth--;
  return result;
}

static int try_materialize_constructor_conversion(CType *type)
{
  SValue source, destination;
  Sym *ctor_func, *ctor_type, *sa;
  int size, align, addr, r2, nb_args;

  if ((type->t & VT_BTYPE) != VT_STRUCT)
    return 0;
  ctor_func = resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                               &vtop->type, 1);
  if (!ctor_func)
    ctor_func = resolve_member_field_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                       &vtop->type, 1);
  if (!ctor_func && (vtop->type.t & VT_ARRAY))
  {
    CType decayed_arg_type = vtop->type;
    decayed_arg_type.t &= ~VT_ARRAY;
    ctor_func = resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                 &decayed_arg_type, 1);
    if (!ctor_func)
      ctor_func = resolve_member_field_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                         &decayed_arg_type, 1);
  }
  if (!ctor_func)
    return 0;
  if ((ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref)
    cprime_error("constructor target is not declared as function");
  if (ctor_func->type.ref->f.func_cpp_explicit)
    return 0;

  source = *vtop;
  vtop--;

  /* The source can be a function result held only in a volatile register.
     Destination member construction happens first and may make calls, so
     leave the source live on the value stack until those calls complete. */
  vpushv(&source);

  size = type_size(type, &align);
  addr = get_temp_local_var(size, align, &r2);
  vset(type, VT_LOCAL | VT_LVAL, addr);
  vtop->r2 = r2;
  call_lifecycle_constructor_members(type, VT_LOCAL | VT_LVAL, addr);
  mk_pointer(&vtop->type);
  gaddrof();

  destination = *vtop;
  vtop--;
  source = *vtop;
  vtop--;

  vpushsym(&ctor_func->type, ctor_func);
  vpushv(&destination);
  ctor_type = ctor_func->type.ref;
  sa = ctor_type->next;
  gfunc_param_typed(ctor_type, sa);
  if (sa)
    sa = sa->next;

  vpushv(&source);
  gfunc_param_typed(ctor_type, sa);
  if (sa)
    sa = sa->next;
  nb_args = 2;
  while (sa)
  {
    int saved_tok = tok;
    CValue saved_tokc = tokc;
    emit_default_arg(ctor_type, sa);
    tok = saved_tok;
    tokc = saved_tokc;
    nb_args++;
    sa = sa->next;
  }

  vcheck_cmp();
  gfunc_call(nb_args);
  drop_leaked_call_target(ctor_func);

  vset(type, VT_LOCAL | VT_LVAL, addr);
  vtop->r2 = r2;
  cpp_register_class_temporary(vtop);
  return 1;
}

static int is_same_template_family_conversion_ctor(CType *class_type,
                                                   CType *arg_type)
{
  CType arg_value_type;
  int class_tok, arg_tok, i, j;

  if (!class_type || !arg_type
      || (class_type->t & VT_BTYPE) != VT_STRUCT)
    return 0;
  arg_value_type = *arg_type;
  arg_value_type.t &= ~VT_RVALUE_REFERENCE;
  if (is_reference_type(&arg_value_type))
    arg_value_type = *pointed_type(&arg_value_type);
  if ((arg_value_type.t & VT_BTYPE) != VT_STRUCT)
    return 0;
  class_tok = get_struct_type_name_tok(class_type);
  arg_tok = get_struct_type_name_tok(&arg_value_type);
  if (!class_tok || !arg_tok || class_tok == arg_tok)
    return 0;
  if (!class_or_inst_has_member_template_name(class_tok, TOK_CONSTRUCTOR1))
    return 0;
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    int class_seen = 0, arg_seen = 0;
    if (!td->is_class)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
    {
      if (td->inst_name_toks[j] == class_tok
          || !strcmp(get_tok_str(td->inst_name_toks[j], NULL),
                     get_tok_str(class_tok, NULL)))
        class_seen = 1;
      if (td->inst_name_toks[j] == arg_tok
          || !strcmp(get_tok_str(td->inst_name_toks[j], NULL),
                     get_tok_str(arg_tok, NULL)))
        arg_seen = 1;
    }
    if (class_seen && arg_seen)
      return 1;
  }
  return 0;
}

static int member_field_func_arg_match_rank(Sym *field, CType *arg_types,
                                            int explicit_arg_count)
{
  int i, rank, total = 0;
  Sym *arg;

  if (!field || (field->type.t & VT_BTYPE) != VT_FUNC || !field->type.ref)
    return -1;
  arg = field->type.ref->next;
  for (i = 0; i < explicit_arg_count; ++i)
  {
    if (!arg)
      return -1;
    rank = call_arg_match_rank(&arg->type, &arg_types[i]);
    if (rank < 0)
      return -1;
    total += rank;
    arg = arg->next;
  }
  for (; arg; arg = arg->next)
    if (!arg->default_arg)
      return -1;
  return total;
}

static Sym *resolve_member_field_func_by_arg_types_filtered(CType *type, int method_tok,
                                                   CType *arg_types,
                                                   int explicit_arg_count, int exclude_explicit)
{
  int struct_tok, best_rank = 0x7fffffff;
  Sym *class_sym, *field, *best = NULL;
  CType lowered_type, struct_type;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  class_sym = struct_find(struct_tok);
  if (!class_sym)
    return NULL;
  for (field = class_sym->next; field; field = field->next)
  {
    int rank;
    if ((field->v & ~SYM_FIELD) != method_tok
        && !(method_tok == TOK_CONSTRUCTOR1
             && (field->v & ~SYM_FIELD) == struct_tok)
        || (field->type.t & VT_BTYPE) != VT_FUNC)
      continue;
    if (exclude_explicit && field->type.ref
        && field->type.ref->f.func_cpp_explicit) continue;
    rank = member_field_func_arg_match_rank(field, arg_types,
                                            explicit_arg_count);
    if (method_tok == tok_alloc_const("operator=") && explicit_arg_count == 1
        && field->type.ref && field->type.ref->next
        && !is_compatible_unqualified_types(type, &arg_types[0]))
    {
      CType param_type = field->type.ref->next->type;
      if (is_reference_type(&param_type))
        param_type = *pointed_type(&param_type);
      if ((param_type.t & VT_BTYPE) == VT_STRUCT
          && (is_compatible_unqualified_types(type, &param_type)
              || (get_struct_type_name_tok(type)
                  && get_struct_type_name_tok(type)
                     == get_struct_type_name_tok(&param_type))))
        continue;
    }
    if (rank < 0 || rank > best_rank)
      continue;
    if (rank == best_rank && best && best != field)
      continue;
    best_rank = rank;
    best = field;
  }
  if (!best)
    return NULL;
  struct_type = *type;
  lowered_type = make_lowered_member_func_type(&struct_type, &best->type);
  return external_global_sym(make_member_func_tok_for_type(struct_tok,
                                                           method_tok,
                                                           &best->type),
                             &lowered_type);
}

static Sym *resolve_member_field_func_by_arg_types(CType *type, int method_tok,
                                                   CType *arg_types,
                                                   int explicit_arg_count)
{
  return resolve_member_field_func_by_arg_types_filtered(type, method_tok,
      arg_types, explicit_arg_count, 0);
}

static int member_func_matches_arg_types(Sym *s, CType *arg_types,
                                         int explicit_arg_count)
{
  int i;
  Sym *arg;

  if (!s || (s->type.t & VT_BTYPE) != VT_FUNC || !s->type.ref)
    return 0;
  arg = s->type.ref->next;
  if (arg)
    arg = arg->next; /* skip implicit this */
  for (i = 0; i < explicit_arg_count; ++i)
  {
    if (!arg)
      return 0;
    if (!call_arg_matches_param_type(&arg->type, &arg_types[i]))
      return 0;
    arg = arg->next;
  }
  for (; arg; arg = arg->next)
    if (!arg->default_arg)
      return 0;
  return 1;
}

static int member_func_exact_arg_types(Sym *s, CType *arg_types,
                                       int explicit_arg_count)
{
  int i;
  Sym *arg;

  if (!s || (s->type.t & VT_BTYPE) != VT_FUNC || !s->type.ref)
    return 0;
  arg = s->type.ref->next;
  if (arg)
    arg = arg->next; /* skip implicit this */
  for (i = 0; i < explicit_arg_count; ++i)
  {
    if (!arg || !call_arg_exact_param_type(&arg->type, &arg_types[i]))
      return 0;
    arg = arg->next;
  }
  for (; arg; arg = arg->next)
    if (!arg->default_arg)
      return 0;
  return 1;
}

static int member_func_arg_match_rank(Sym *s, CType *arg_types,
                                      int explicit_arg_count)
{
  int i, rank, total = 0;
  Sym *arg;

  if (!s || (s->type.t & VT_BTYPE) != VT_FUNC || !s->type.ref)
    return -1;
  arg = s->type.ref->next;
  if (arg)
    arg = arg->next; /* skip implicit this */
  for (i = 0; i < explicit_arg_count; ++i)
  {
    if (!arg)
      return -1;
    rank = call_arg_match_rank(&arg->type, &arg_types[i]);
    if (rank < 0)
      return -1;
    total += rank;
    arg = arg->next;
  }
  for (; arg; arg = arg->next)
    if (!arg->default_arg)
      return -1;
  return total;
}

static int lowered_member_func_single_param_is_self(CType *type, Sym *s)
{
  Sym *arg;
  CType param_type;

  if (!s || (s->type.t & VT_BTYPE) != VT_FUNC || !s->type.ref)
    return 0;
  arg = s->type.ref->next;
  if (arg)
    arg = arg->next; /* skip implicit this */
  if (!arg || arg->next)
    return 0;
  param_type = arg->type;
  if (is_reference_type(&param_type))
    param_type = *pointed_type(&param_type);
  return (param_type.t & VT_BTYPE) == VT_STRUCT
         && is_compatible_unqualified_types(type, &param_type);
}

static int same_lowered_member_func_signature(CType *type1, CType *type2)
{
  Sym *arg1, *arg2;

  if (!type1 || !type1->ref || !type2 || !type2->ref
      || (type1->t & VT_BTYPE) != VT_FUNC
      || (type2->t & VT_BTYPE) != VT_FUNC)
    return 0;
  arg1 = type1->ref->next;
  arg2 = type2->ref->next;
  for (;;)
  {
    if (!arg1 || !arg2)
      return arg1 == arg2;
    if ((arg1->type.t & VT_RVALUE_REFERENCE)
        != (arg2->type.t & VT_RVALUE_REFERENCE))
      return 0;
    if (!is_compatible_types(&arg1->type, &arg2->type))
      return 0;
    arg1 = arg1->next;
    arg2 = arg2->next;
  }
}

/* Explicit template arguments select specializations before ordinary
   conversion ranking. Previously instantiated siblings can have identical
   function parameter types while denoting different bodies. */
static int member_function_matches_explicit_arguments(int function_tok,
                                                       int argument_key)
{
  MemberFunctionScope *scope;
  if (!argument_key) return 1;
  for (scope = member_function_scopes[(unsigned)function_tok & 1023];
       scope; scope = scope->next)
    if (scope->function_tok == function_tok)
    {
      TemplateMemberDef *definition = scope->template_definition;
      int i;
      if (!scope->is_function_template || !definition) return 0;
      for (i = 0; i < definition->nb_inst; ++i)
        if (definition->inst_func_toks[i] == function_tok
            && definition->inst_pack_toks
            && definition->inst_pack_toks[i] == argument_key)
          return 1;
      return 0;
    }
  return 0;
}

static Sym *resolve_member_func_by_arg_types_filtered(CType *type, int method_tok,
                                             CType *arg_types,
                                             int explicit_arg_count, int exclude_explicit)
{
  int struct_tok;
  MemberFuncOverload *o;
  Sym *match = NULL;
  Sym *const_match = NULL;
  int receiver_const = is_lifecycle_member_tok(method_tok)
                         ? 0 : member_receiver_is_const(type);
  int best_omitted_defaults = 0x7fffffff;
  int best_arg_rank = 0x7fffffff;
  int has_exact_copy_ctor = 0;
  int needs_template_copy_body = 0;
  TokenString *entry_macro_stack = macro_stack;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  if (method_tok == TOK_CONSTRUCTOR1 && arg_types
      && explicit_arg_count == 1)
    for (o = member_func_candidates(struct_tok, method_tok);
         o; o = o->bucket_next)
    {
      if (profile_scans_enabled) ++profile_overload_scans;
      Sym *s;
      CType source_type = arg_types[0];
      int source_struct_tok;
      int source_is_derived;
      if (o->struct_tok != struct_tok || o->method_tok != method_tok)
        continue;
      s = global_symbol_find(o->mangled_tok);
      if (!s)
        s = sym_find(o->mangled_tok);
      if (exclude_explicit && s && s->type.ref
          && s->type.ref->f.func_cpp_explicit) continue;
      source_type.t &= ~VT_RVALUE_REFERENCE;
      if (is_reference_type(&source_type))
        source_type = *pointed_type(&source_type);
      source_struct_tok = get_struct_type_name_tok(&source_type);
      source_is_derived = source_struct_tok
                          && class_has_base(source_struct_tok, struct_tok);
      if (s && lowered_member_func_single_param_is_self(type, s)
          && (source_is_derived
              || member_func_matches_arg_types(s, arg_types,
                                               explicit_arg_count)))
      {
        has_exact_copy_ctor = 1;
        break;
      }
    }
  if (has_exact_copy_ctor)
  {
    TemplateDef *class_td = find_class_template_def_for_class_tok(struct_tok);
    if (class_td)
    {
      TemplateMemberDef *md;
      for (md = template_member_candidates(class_td->name_tok,
                                            TOK_CONSTRUCTOR1);
           md; md = md->bucket_next)
        if (md->class_tok == class_td->name_tok
            && template_member_def_method_tok(md) == TOK_CONSTRUCTOR1
            && template_member_def_is_scoped(md)
            && template_member_def_param_count(md, TOK_CONSTRUCTOR1) == 1)
        {
          needs_template_copy_body = 1;
          break;
        }
    }
  }
  if (!has_exact_copy_ctor || needs_template_copy_body)
    instantiate_template_member_for_call(type, method_tok, arg_types,
                                         explicit_arg_count);
  if (method_tok == TOK_CONSTRUCTOR1 && nb_pending_member_funcs)
    compile_pending_member_funcs(0);
  if (getenv("CPC_TRACE_RESOLVE_STACK")
      && funcname && strstr(funcname, getenv("CPC_TRACE_RESOLVE_STACK"))
      && macro_stack != entry_macro_stack)
    fprintf(stderr,
            "CPC_RESOLVE_STACK func=%s recv=%s method=%s argc=%d arg0=%s before=%p after=%p top_len=%d\n",
            funcname, get_tok_str(struct_tok, NULL),
            get_tok_str(method_tok, NULL), explicit_arg_count,
            arg_types && explicit_arg_count > 0
              ? get_tok_str(get_struct_type_name_tok(&arg_types[0]), NULL)
              : "<none>",
            (void *)entry_macro_stack, (void *)macro_stack,
            macro_stack ? macro_stack->len : -1);
  for (o = member_func_candidates(struct_tok, method_tok); o;)
  {
    MemberFuncOverload *next_o = o->bucket_next;
    Sym *s;
    if (profile_scans_enabled) ++profile_overload_scans;
    if (getenv("CPC_TRACE_CLBITREF")
        && struct_tok >= TOK_UIDENT
        && strstr(get_tok_str(struct_tok, NULL), "clBitRef")
        && method_tok == tok_alloc_const("operator="))
      fprintf(stderr, "CPC_CLBITREF_TYPE_CAND recv=%s cand=%s argc=%d min=%d explicit=%d struct_match=%d method_match=%d\n",
              get_tok_str(struct_tok, NULL),
              get_tok_str(o->mangled_tok, NULL), explicit_arg_count,
              o->min_arg_count, o->explicit_arg_count,
              o->struct_tok == struct_tok, o->method_tok == method_tok);
    if (o->struct_tok != struct_tok || o->method_tok != method_tok
        || explicit_arg_count < o->min_arg_count
        || explicit_arg_count > o->explicit_arg_count)
      goto next_overload;
    if (!member_function_matches_explicit_arguments(o->mangled_tok,
                                                     explicit_member_template_arg_tok))
      goto next_overload;
    if (!member_overload_receiver_ok(o, receiver_const, type))
    {
      if (getenv("CPC_TRACE_CLBITREF")
          && struct_tok >= TOK_UIDENT
          && strstr(get_tok_str(struct_tok, NULL), "clBitRef")
          && method_tok == tok_alloc_const("operator="))
        fprintf(stderr, "CPC_CLBITREF_SKIP_RECEIVER cand=%s recv_const=%d o_const=%d type_t=%04x\n",
                get_tok_str(o->mangled_tok, NULL), receiver_const,
                o->is_const, type->t);
      goto next_overload;
    }
    s = global_symbol_find(o->mangled_tok);
    if (!s)
      s = sym_find(o->mangled_tok);
    if (!s && o->func_type.ref)
      s = external_global_sym(o->mangled_tok, &o->func_type);
    if (s)
      use_overload_func_type(s, &o->func_type);
    if (exclude_explicit && s && s->type.ref
        && s->type.ref->f.func_cpp_explicit) goto next_overload;
    if (method_tok == TOK_CONSTRUCTOR1 && explicit_arg_count == 1
        && !is_compatible_unqualified_types(type, &arg_types[0])
        && lowered_member_func_single_param_is_self(type, s))
      goto next_overload;
    {
      int arg_rank = member_func_arg_match_rank(s, arg_types,
                                                explicit_arg_count);
      if (getenv("CPC_TRACE_CLBITREF")
          && struct_tok >= TOK_UIDENT
          && strstr(get_tok_str(struct_tok, NULL), "clBitRef")
          && method_tok == tok_alloc_const("operator="))
        fprintf(stderr, "CPC_CLBITREF_RANK cand=%s rank=%d s=%p\n",
                get_tok_str(o->mangled_tok, NULL), arg_rank, (void *)s);
      if (arg_rank < 0)
        goto next_overload;
      if (arg_rank > best_arg_rank)
        goto next_overload;
      if (arg_rank < best_arg_rank)
      {
        match = NULL;
        const_match = NULL;
        best_omitted_defaults = 0x7fffffff;
        best_arg_rank = arg_rank;
      }
    }
    if (!member_func_matches_arg_types(s, arg_types, explicit_arg_count))
      goto next_overload;
    {
      Sym **slot = o->is_const ? &const_match : &match;
      if (*slot && *slot != s)
      {
        int enum_preference = cpp_compare_enum_promotion_overloads(
          s, *slot, arg_types, explicit_arg_count, 1);
        int previous_template = member_function_is_template((*slot)->v);
        int candidate_template = member_function_is_template(s->v);
        if (enum_preference < 0)
          goto next_overload;
        if (enum_preference > 0) {
          *slot = s;
          goto next_overload;
        }
        /* Equal implicit-object and explicit-argument conversion sequences
           prefer an ordinary member over a function-template specialization. */
        if (!previous_template && candidate_template)
          goto next_overload;
        if (previous_template && !candidate_template)
        {
          *slot = s;
          goto next_overload;
        }
        if (previous_template && candidate_template)
        {
          int order = compare_member_function_template_order(s->v, (*slot)->v);
          if (order > 0)
          {
            *slot = s;
            goto next_overload;
          }
          if (order < 0)
            goto next_overload;
        }
        if (same_lowered_member_func_signature(&(*slot)->type, &s->type))
          goto next_overload;
      }
    }
    member_overload_note_ranked_match(&match, &const_match,
                                      &best_omitted_defaults, s,
                                      o->is_const,
                                      o->explicit_arg_count - explicit_arg_count);
next_overload:
    o = next_o;
  }
  match = member_overload_pick_match(match, const_match, receiver_const);
  if (getenv("CPC_TRACE_MATRIX")
      && strstr(get_tok_str(struct_tok, NULL), "clMatrix4x4")
      && method_tok == tok_alloc_const("operator*")
      && arg_types && explicit_arg_count == 1)
    fprintf(stderr, "CPC_MATRIX_SELECTED arg_t=%04x arg=%s match=%s ret=%s\n",
            arg_types[0].t,
            get_tok_str(get_struct_type_name_tok(&arg_types[0]), NULL),
            match ? get_tok_str(match->v, NULL) : "<none>",
            match && match->type.ref
              ? get_tok_str(get_struct_type_name_tok(&match->type.ref->type), NULL)
              : "<none>");
  if (match)
  {
    instantiate_template_member_body_for_func_tok(type, method_tok, match->v);
    return match;
  }
  /* Constructors/destructors belong to their declaring class. Implicit
     lifecycle work visits every base; it cannot select one inherited body. */
  if (is_lifecycle_member_tok(method_tok)) return NULL;
  {
    ClassBaseInfo *info;
    for (info = class_base_infos; info; info = info->next)
    {
      CType base_type;
      Sym *base_match;

      if (info->class_tok != struct_tok)
        continue;
      if (!make_class_type_from_tok(&base_type, info->base_tok))
        continue;
      base_type.t |= type->t & VT_CONSTANT;
      base_match = resolve_member_func_by_arg_types(&base_type, method_tok,
                                                    arg_types,
                                                    explicit_arg_count);
      if (base_match)
        return base_match;
    }
  }
  return NULL;
}

static Sym *resolve_member_func_by_arg_types(CType *type, int method_tok,
                                             CType *arg_types,
                                             int explicit_arg_count)
{
  return resolve_member_func_by_arg_types_filtered(type, method_tok,
      arg_types, explicit_arg_count, 0);
}

/* Copy-initialization excludes explicit constructors before overload ranking. */
static Sym *resolve_implicit_constructor_func(CType *type, CType *arguments,
                                               int argument_count)
{
  Sym *result = resolve_member_func_by_arg_types_filtered(type, TOK_CONSTRUCTOR1,
      arguments, argument_count, 1);
  return result ? result : resolve_member_field_func_by_arg_types_filtered(type,
      TOK_CONSTRUCTOR1, arguments, argument_count, 1);
}

static int same_concrete_template_instantiation(CType *type1, CType *type2)
{
  int tok1, tok2, i, j, k;

  if (!type1 || !type2
      || (type1->t & VT_BTYPE) != VT_STRUCT
      || (type2->t & VT_BTYPE) != VT_STRUCT)
    return 0;
  tok1 = get_struct_type_name_tok(type1);
  tok2 = get_struct_type_name_tok(type2);
  if (tok1 == tok2)
    return 1;
  for (i = 0; i < nb_template_defs; ++i)
  {
    TemplateDef *td = template_defs[i];
    int inst1 = -1, inst2 = -1;
    if (!td->is_class)
      continue;
    for (j = 0; j < td->nb_inst; ++j)
    {
      if (td->inst_name_toks[j] == tok1)
        inst1 = j;
      if (td->inst_name_toks[j] == tok2)
        inst2 = j;
    }
    if (inst1 < 0 || inst2 < 0)
      continue;
    for (k = 0; k < td->nb_type_params; ++k)
      if (!template_type_arg_toks_match(
            td->inst_type_toks[inst1 * td->nb_type_params + k],
            td->inst_type_toks[inst2 * td->nb_type_params + k]))
        return 0;
    return 1;
  }
  return 0;
}

static int exact_template_signature_type_inner(CType *type1, CType *type2,
                                                int compare_qualifiers)
{
  int bt = type1->t & VT_BTYPE;
  if (bt != (type2->t & VT_BTYPE)
      || ((type1->t ^ type2->t) & (VT_REFERENCE | VT_RVALUE_REFERENCE)))
    return 0;
  if (compare_qualifiers
      && ((type1->t ^ type2->t) & (VT_CONSTANT | VT_VOLATILE)))
    return 0;
  if (bt == VT_PTR)
    return exact_template_signature_type_inner(pointed_type(type1),
                                                pointed_type(type2), 1);
  if (bt == VT_STRUCT)
    return same_concrete_template_instantiation(type1, type2);
  return is_compatible_unqualified_types(type1, type2);
}

static int exact_template_signature_type(CType *type1, CType *type2)
{
  return exact_template_signature_type_inner(type1, type2, 0);
}

static int member_func_has_param_signature(Sym *s, CType *func_type)
{
  Sym *member_arg, *def_arg;

  if (!s || (s->type.t & VT_BTYPE) != VT_FUNC || !s->type.ref
      || !func_type || (func_type->t & VT_BTYPE) != VT_FUNC || !func_type->ref)
    return 0;
  member_arg = s->type.ref->next;
  if (member_arg)
    member_arg = member_arg->next; /* skip implicit this */
  def_arg = func_type->ref->next;
  for (;;)
  {
    if (!member_arg || !def_arg)
      return member_arg == def_arg;
    if (!exact_template_signature_type(&member_arg->type, &def_arg->type))
      return 0;
    member_arg = member_arg->next;
    def_arg = def_arg->next;
  }
}

static Sym *resolve_member_func_by_param_signature(CType *type, int method_tok,
                                                   CType *func_type)
{
  int struct_tok;
  MemberFuncOverload *o;
  Sym *match = NULL;
  Sym *const_match = NULL;
  int receiver_const = is_lifecycle_member_tok(method_tok)
                         ? 0 : member_receiver_is_const(type);

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  for (o = member_func_candidates(struct_tok, method_tok);
       o; o = o->bucket_next)
  {
    Sym *s;
    if (profile_scans_enabled) ++profile_other_overload_scans[5];
    if (o->struct_tok != struct_tok || o->method_tok != method_tok)
      continue;
    /* An ordinary out-of-class definition cannot bind to an instantiated
       member template just because their substituted parameter types match. */
    {
      const CppMemberDeclInfo *declaration = lookup_cpp_member_decl(o->mangled_tok);
      if (declaration && declaration->is_template) continue;
    }
    if (o->is_const != receiver_const
        || o->func_type.ref->f.func_ref_qualifier !=
           ((func_type->t & VT_FUNC_RREF) ? 2 : (func_type->t & VT_FUNC_LREF) ? 1 : 0))
      continue;
    s = sym_find(o->mangled_tok);
    if (!s)
      s = global_symbol_find(o->mangled_tok);
    if (s)
      use_overload_func_type(s, &o->func_type);
    if (!member_func_has_param_signature(s, func_type))
      continue;
    member_overload_note_match(&match, &const_match, s, o->is_const);
  }
  return member_overload_pick_match(match, const_match, receiver_const);
}

static int add_ctype_tokens(TokenString *str, CType *type)
{
  int t = type->t;
  int bt = t &VT_BTYPE;
  CType pt;

  if ((t & VT_ARRAY) || bt == VT_FUNC
      || ((IS_ENUM(t) || bt == VT_STRUCT) && type->ref
          && (type->ref->v & ~SYM_STRUCT) >= SYM_FIRST_ANOM))
  {
    /* Array and function suffixes belong after the declarator. A stable
       alias preserves nested extents and signatures through pointers and
       references, including references to captured function pointers. An
       anonymous record likewise has no tag that can be replayed as source. */
    int alias = template_ctype_typedef_tok(type);
    if (!alias)
      return 0;
    tok_str_add(str, alias);
    return 1;
  }
  if (bt == VT_PTR)
  {
    pt = *pointed_type(type);
    if (!add_ctype_tokens(str, &pt))
      return 0;
    if (t & VT_RVALUE_REFERENCE)
      tok_str_add(str, TOK_LAND);
    else
      tok_str_add(str, (t & VT_REFERENCE) ? '&' : '*');
    if (t & VT_CONSTANT)
      tok_str_add(str, TOK_CONST1);
    if (t & VT_VOLATILE)
      tok_str_add(str, TOK_VOLATILE1);
    return 1;
  }

  if (t & VT_CONSTANT)
    tok_str_add(str, TOK_CONST1);
  if (t & VT_VOLATILE)
    tok_str_add(str, TOK_VOLATILE1);

  if (IS_ENUM(t))
  {
    int enum_tok;
    if (!type->ref)
      return 0;
    enum_tok = type->ref->v & ~SYM_STRUCT;
    if (enum_tok < TOK_UIDENT)
      return 0;
    tok_str_add(str, TOK_ENUM);
    tok_str_add(str, enum_tok);
    return 1;
  }

  if (CHARACTER_TYPE_KIND(t)) {
    tok_str_add(str, tok_alloc_const(CHARACTER_TYPE_NAME(t)));
    return 1;
  }
  if (bt == VT_NULLPTR)
  {
    tok_str_add(str, tok_alloc_const("decltype"));
    tok_str_add(str, '(');
    tok_str_add(str, TOK_NULLPTR);
    tok_str_add(str, ')');
    return 1;
  }
  if (bt == VT_LLONG)
  {
    if ((t & VT_UNSIGNED) && bt != VT_FLOAT && bt != VT_DOUBLE)
      tok_str_add(str, TOK_UNSIGNED);
    tok_str_add(str, TOK_LONG);
    tok_str_add(str, TOK_LONG);
    tok_str_add(str, TOK_INT);
    return 1;
  }

  if ((t & VT_DEFSIGN) && bt == VT_BYTE && !(t & VT_UNSIGNED))
    tok_str_add(str, TOK_SIGNED1);
  if ((t & VT_UNSIGNED) && bt != VT_FLOAT && bt != VT_DOUBLE
      && !(t & VT_WCHAR_T))
    tok_str_add(str, TOK_UNSIGNED);
  if (t & VT_LONG)
    tok_str_add(str, TOK_LONG);
  if (bt == VT_VOID)
    tok_str_add(str, TOK_VOID);
  else if (bt == VT_BYTE)
    tok_str_add(str, TOK_CHAR);
  else if (bt == VT_SHORT)
  {
    if (t & VT_WCHAR_T)
      tok_str_add(str, tok_alloc_const("wchar_t"));
    else
      tok_str_add(str, TOK_SHORT);
  }
  else if (bt == VT_INT || bt == VT_LLONG)
    tok_str_add(str, TOK_INT);
  else if (bt == VT_FLOAT)
    tok_str_add(str, TOK_FLOAT);
  else if (bt == VT_DOUBLE)
    tok_str_add(str, TOK_DOUBLE);
  else if (bt == VT_BOOL)
    tok_str_add(str, TOK_BOOL);
  else if (bt == VT_STRUCT)
  {
    int struct_tok = get_struct_type_name_tok(type);
    if (!struct_tok)
      return 0;
    tok_str_add(str, TOK_STRUCT);
    tok_str_add(str, struct_tok);
  }
  else
    return 0;
  return 1;
}

static int add_ctype_declarator_tokens(TokenString *str, CType *type,
                                       int name_tok)
{
  if ((type->t & VT_BTYPE) == VT_PTR
      && !(type->t & (VT_REFERENCE | VT_RVALUE_REFERENCE)))
  {
    CType target = *pointed_type(type);
    if ((target.t & VT_BTYPE) == VT_FUNC && target.ref)
    {
      Sym *arg;
      int first = 1;
      if (!add_ctype_tokens(str, &target.ref->type))
        return 0;
      tok_str_add(str, '(');
      tok_str_add(str, '*');
      if (name_tok >= TOK_UIDENT)
        tok_str_add(str, name_tok);
      tok_str_add(str, ')');
      tok_str_add(str, '(');
      for (arg = target.ref->next; arg; arg = arg->next)
      {
        if (!first)
          tok_str_add(str, ',');
        if (!add_ctype_tokens(str, &arg->type))
          return 0;
        first = 0;
      }
      if (target.ref->f.func_type == FUNC_ELLIPSIS)
      {
        if (!first)
          tok_str_add(str, ',');
        tok_str_add(str, TOK_DOTS);
      }
      tok_str_add(str, ')');
      return 1;
    }
  }
  if (!add_ctype_tokens(str, type))
    return 0;
  if (name_tok >= TOK_UIDENT)
    tok_str_add(str, name_tok);
  return 1;
}

static CType make_lowered_member_func_type(CType *struct_type, CType *func_type)
{
  CType lowered_type, return_type, this_type, arg_type;
  Sym *fref, *param, *arg, *last;
  Sym *saved_ls;

  return_type = func_type->ref->type;
  lowered_type.t = VT_FUNC;
  if (template_type_has_internal_linkage(struct_type, 0))
    lowered_type.t |= VT_STATIC;
  /* Same as make_lifecycle_func_type: keep these type symbols off a live
     local scope so scope pops cannot unlink the 'this' parameter symbols. */
  saved_ls = local_stack;
  local_stack = NULL;
  lowered_type.ref = fref = sym_push(SYM_FIELD, &return_type, 0, 0);
  fref->f = func_type->ref->f;
  fref->f.func_cpp_member = 1;
  if (func_type->t & (VT_FUNC_LREF | VT_FUNC_RREF))
    fref->f.func_ref_qualifier = (func_type->t & VT_FUNC_RREF) ? 2 : 1;

  this_type = *struct_type;
  if (func_type->t & VT_CONSTANT)
    this_type.t |= VT_CONSTANT;
  mk_pointer(&this_type);
  param = sym_push(SYM_FIELD, &this_type, VT_LOCAL | VT_LVAL, 0);
  param->v = tok_alloc_const("this");
  fref->next = param;
  last = param;

  for (arg = func_type->ref->next; arg; arg = arg->next)
  {
    arg_type = arg->type;
    param = sym_push(SYM_FIELD, &arg_type, arg->r, arg->c);
    param->v = arg->v;
    param->default_arg = arg->default_arg;
    last->next = param;
    last = param;
  }
  local_stack = saved_ls;

  return lowered_type;
}

static int append_nested_lambda_tokens(TokenString *destination,
                                       TokenString *source, int *index)
{
  int i = *index, braces = 0, in_body = 0;
  if (!is_cpp_lambda_marker(source->str[i]))
    return 0;
  /* A lambda introduces its own member/capture scope. Qualifying its names
     as members of the enclosing class corrupts both [capture] and its body.
     The normal lambda parser performs lookup when this expression replays. */
  for (; i < source->len && source->str[i] != TOK_EOF; ++i)
  {
    int token = source->str[i];
    if (TOK_HAS_VALUE(token))
    {
      tok_str_add_record(destination, source->str, source->len, &i);
      continue;
    }
    tok_str_add(destination, token);
    if (token == '{') { ++braces; in_body = 1; }
    else if (token == '}' && in_body && !--braces)
    {
      *index = i;
      return 1;
    }
  }
  cprime_error("unterminated lambda body");
  return 0;
}

static void add_pending_member_func(CType *struct_type, int method_tok,
                                    CType *func_type, TokenString *body,
                                    int is_auto_return)
{
  int struct_tok, mangled_tok, i;
  int is_local_class_member = local_stack || local_scope;
  int this_tok, public_this_tok;
  Sym *arg;
  PendingMemberFunc *pm;
  TokenString *str;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("inline member functions require a named struct/class");
  mangled_tok = make_member_func_tok_for_type(struct_tok, method_tok, func_type);
  note_cpp_member_decl(struct_tok, method_tok, mangled_tok, func_type, 0, 0);
  this_tok = make_joined_name_tok(tok_alloc_const("__cprime_this"),
                                   mangled_tok, "_", "");
  public_this_tok = tok_alloc_const("this");
  {
    CType lowered_type = make_lowered_member_func_type(struct_type, func_type);
  note_member_func_overload(struct_tok, method_tok, mangled_tok, &lowered_type);
  external_global_sym(mangled_tok, &lowered_type);
  }

  str = tok_str_alloc();
  if (!is_local_class_member)
  {
    tok_str_add(str, TOK_STATIC);
    tok_str_add(str, TOK_INLINE1);
  }
  if (!add_ctype_tokens(str, &func_type->ref->type))
    cprime_error("unsupported member function return type");
  tok_str_add(str, mangled_tok);
  tok_str_add(str, '(');
  tok_str_add(str, TOK_STRUCT);
  tok_str_add(str, struct_tok);
  if (func_type->t & VT_CONSTANT)
    tok_str_add(str, TOK_CONST1);
  tok_str_add(str, '*');
  tok_str_add(str, this_tok);
  for (arg = func_type->ref->next; arg; arg = arg->next)
  {
    tok_str_add(str, ',');
    if (!add_ctype_declarator_tokens(str, &arg->type,
                                     arg->v & ~SYM_FIELD))
      cprime_error("unsupported member function parameter type");
  }
  tok_str_add(str, ')');
  append_function_exception_spec(str, &func_type->ref->f);
  prepare_saved_cpp_lambdas(body);
  for (i = 0; i < body->len && body->str[i] != TOK_EOF; ++i)
  {
    int prev_body_tok = 0, pi;
    if (append_nested_lambda_tokens(str, body, &i))
      continue;
    if (TOK_HAS_VALUE(body->str[i]))
    {
      tok_str_add_record(str, body->str, body->len, &i);
      continue;
    }
    for (pi = i - 1; pi >= 0; --pi)
    {
      if (body->str[pi] == TOK_LINENUM)
      {
        --pi;
        continue;
      }
      prev_body_tok = body->str[pi];
      break;
    }
    if (body->str[i] >= TOK_UIDENT
        && prev_body_tok != '.' && prev_body_tok != TOK_ARROW
        && prev_body_tok != ':')
    {
      int prev_i = tok_str_prev_token_index(body->str, body->len, i);
      int next_i = tok_str_next_non_linenum_index(body->str, body->len, i + 1);
      int is_param = 0;
      Sym *param;
      Sym *static_member =
        find_static_member_by_class_try(struct_tok, body->str[i], NULL);
      for (param = func_type->ref ? func_type->ref->next : NULL;
           param; param = param->next)
        if ((param->v & ~SYM_FIELD) == body->str[i])
        {
          is_param = 1;
          break;
        }
      if (static_member
          && (static_member->type.t & VT_BTYPE) != VT_FUNC
          && !is_param
          && !body_identifier_is_decl_target(body, i, prev_i, next_i))
      {
        if (!tok_str_add_integer_const_sym(str, static_member))
          tok_str_add(str, make_static_member_tok(struct_tok, body->str[i]));
        continue;
      }
    }
    if (body->str[i] >= TOK_UIDENT
        && (i == 0 || body->str[i - 1] != ':')
        && (i + 1 >= body->len || body->str[i + 1] != ':')
        && prev_body_tok != '~'
        && class_tok_matches_unqualified_name(struct_tok, body->str[i]))
    {
      tok_str_add(str, struct_tok);
      continue;
    }
    if (body->str[i] >= TOK_UIDENT
        && tok_str_next_non_linenum_index(body->str, body->len, i + 1)
             < body->len
        && body->str[tok_str_next_non_linenum_index(body->str, body->len,
                                                     i + 1)] == '('
        && prev_body_tok != '.'
        && prev_body_tok != TOK_ARROW
        && prev_body_tok != ':'
        && body->str[i] != struct_tok
        && (class_has_static_member_func(struct_tok, body->str[i])
            || type_has_member_func_name(struct_type, body->str[i])))
    {
      int call_paren_i = tok_str_next_non_linenum_index(body->str,
                                                        body->len, i + 1);
      int call_arg_count = count_call_arg_tokens_in_str(body, call_paren_i);
      if (class_has_static_member_func(struct_tok, body->str[i])
          && !member_overload_exists_for_call(struct_type, body->str[i],
                                              call_arg_count))
      {
        /* Static member calls cannot go through this->; qualify them as
           Class::Member. */
        tok_str_add(str, struct_tok);
        tok_str_add(str, ':');
        tok_str_add(str, ':');
      }
      else
      {
        int owner_tok = member_func_owner_for_call(struct_type, body->str[i],
                                                   call_arg_count);
        if (!owner_tok)
        {
          int member_ofs, field_owner_tok = 0;
          Sym *member_field =
            find_field_try_with_owner(struct_type, body->str[i],
                                      &member_ofs, &field_owner_tok);
          if (member_field
              && ((member_field->type.t & VT_BTYPE) == VT_FUNC))
            owner_tok = field_owner_tok ? field_owner_tok : struct_tok;
        }
        else if (owner_tok == struct_tok)
        {
        }
        else
        {
          tok_str_add(str, '(');
          tok_str_add(str, '(');
          tok_str_add(str, TOK_STRUCT);
          tok_str_add(str, owner_tok);
          tok_str_add(str, '*');
          tok_str_add(str, ')');
          tok_str_add(str, this_tok);
          tok_str_add(str, ')');
        }
        if (owner_tok == struct_tok)
        {
          tok_str_add(str, this_tok);
          tok_str_add(str, TOK_ARROW);
        }
        else if (owner_tok)
          tok_str_add(str, TOK_ARROW);
      }
    }
    if (body->str[i] == public_this_tok)
      tok_str_add(str, this_tok);
    else
      tok_str_add(str, body->str[i]);
  }
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  pm->token_filter_valid = 0;
  pm->struct_tok = struct_tok;
  pm->func_tok = is_auto_return ? mangled_tok : 0;
  pm->auto_return_state = is_auto_return ? 1 : 0;
  pm->auto_return_kind = is_auto_return;
  pm->is_local_class_member = is_local_class_member;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static Sym *declare_member_func(CType *struct_type, int method_tok,
                                CType *func_type)
{
  int struct_tok, mangled_tok;
  CType lowered_type;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("member functions require a named struct/class");
  mangled_tok = make_member_func_tok_for_type(struct_tok, method_tok, func_type);
  note_cpp_member_decl(struct_tok, method_tok, mangled_tok, func_type, 0, 0);
  lowered_type = make_lowered_member_func_type(struct_type, func_type);
  note_member_func_overload(struct_tok, method_tok, mangled_tok, &lowered_type);
  return external_global_sym(mangled_tok, &lowered_type);
}

static Sym *declare_static_member_func(CType *struct_type, int method_tok,
                                       CType *func_type)
{
  int struct_tok, base_tok, mangled_tok;
  CType static_type;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("static member functions require a named struct/class");
  base_tok = make_static_member_tok(struct_tok, method_tok);
  static_type = *func_type;
  static_type.t &= ~VT_STATIC;
  if (template_type_has_internal_linkage(struct_type, 0))
    static_type.t |= VT_STATIC;
  mangled_tok = make_static_member_func_tok_for_type(base_tok, &static_type);
  note_cpp_member_decl(struct_tok, method_tok, mangled_tok, &static_type, 1, 0);
  note_member_function_scope(mangled_tok, struct_tok);
  note_free_func_overload(base_tok, mangled_tok, &static_type);
  if (!sym_find(base_tok) && !global_symbol_find(base_tok))
    external_global_sym(base_tok, &static_type);
  return external_global_sym(mangled_tok, &static_type);
}

static void add_pending_static_member_func(CType *struct_type, int method_tok,
                                            CType *func_type, TokenString *body,
                                            int is_auto_return)
{
  int struct_tok, base_tok, mangled_tok, i;
  int is_local_class_member = local_stack || local_scope;
  Sym *arg;
  CType static_type;
  PendingMemberFunc *pm;
  TokenString *str;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("inline static member functions require a named struct/class");

  declare_static_member_func(struct_type, method_tok, func_type);

  base_tok = make_static_member_tok(struct_tok, method_tok);
  static_type = *func_type;
  static_type.t &= ~VT_STATIC;
  mangled_tok = make_static_member_func_tok_for_type(base_tok, &static_type);
  if (is_auto_return && mangled_tok == base_tok)
    mark_unmangled_free_func_overload_provisional(base_tok);

  str = tok_str_alloc();
  if (!is_local_class_member)
  {
    tok_str_add(str, TOK_STATIC);
    tok_str_add(str, TOK_INLINE1);
  }
  if (!add_ctype_tokens(str, &func_type->ref->type))
    cprime_error("unsupported static member function return type");
  tok_str_add(str, mangled_tok);
  tok_str_add(str, '(');
  for (arg = func_type->ref->next; arg; arg = arg->next)
  {
    if (arg != func_type->ref->next)
      tok_str_add(str, ',');
    if (!add_ctype_declarator_tokens(str, &arg->type,
                                     arg->v & ~SYM_FIELD))
      cprime_error("unsupported static member function parameter type");
  }
  tok_str_add(str, ')');
  append_function_exception_spec(str, &func_type->ref->f);
  for (i = 0; i < body->len && body->str[i] != TOK_EOF; ++i)
  {
    int bt = body->str[i];
    int prev_i = tok_str_prev_token_index(body->str, body->len, i);
    int next_i = tok_str_next_non_linenum_index(body->str, body->len, i + 1);
    if (TOK_HAS_VALUE(bt))
    {
      tok_str_add_record(str, body->str, body->len, &i);
      continue;
    }
    if (bt >= TOK_UIDENT
        && (prev_i < 0 || (body->str[prev_i] != '.'
                           && body->str[prev_i] != TOK_ARROW
                           && body->str[prev_i] != ':'))
        && (next_i >= body->len || body->str[next_i] != '(')
        && !body_identifier_is_decl_target(body, i, prev_i, next_i))
    {
      int is_param = 0;
      Sym *param;
      Sym *static_member =
        find_static_member_by_class_try(struct_tok, bt, NULL);
      for (param = func_type->ref ? func_type->ref->next : NULL;
           param; param = param->next)
        if ((param->v & ~SYM_FIELD) == bt)
        {
          is_param = 1;
          break;
        }
      if (static_member
          && (static_member->type.t & VT_BTYPE) != VT_FUNC
          && !is_param)
      {
        if (!tok_str_add_integer_const_sym(str, static_member))
          tok_str_add(str, make_static_member_tok(struct_tok, bt));
        continue;
      }
    }
    if (bt >= TOK_UIDENT
        && (i == 0 || (body->str[i - 1] != '.'
                       && body->str[i - 1] != TOK_ARROW
                       && body->str[i - 1] != ':'))
        && (i + 1 < body->len
            && (body->str[i + 1] == '('
                || body->str[i + 1] == TOK_LT
                || body->str[i + 1] == '<'))
        && class_has_static_member_func(struct_tok, bt))
    {
      /* Static bodies have no this pointer: qualify nested static member
         and member-template calls as Class::Member so the call resolves
         through the static overload table. */
      tok_str_add(str, struct_tok);
      tok_str_add(str, ':');
      tok_str_add(str, ':');
      tok_str_add(str, bt);
      continue;
    }
    tok_str_add(str, bt);
  }
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  pm->token_filter_valid = 0;
  pm->func_tok = mangled_tok;
  pm->struct_tok = struct_tok;
  pm->is_static_member = 1;
  pm->auto_return_state = is_auto_return ? 1 : 0;
  pm->auto_return_kind = is_auto_return;
  pm->is_local_class_member = is_local_class_member;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

/* A member with a deduced return type must use the completed class
   scope and ordinary expression semantics. Defer this work until a selected
   call needs the interface, or until its pending definition is emitted. */
static void deduce_pending_member_return(int function_tok)
{
  PendingMemberFunc *pending = NULL;
  const CppMemberDeclInfo *member;
  TokenString *probe, *definition;
  CType result;
  Sym *function;
  int i, name_at = -1;
  for (i = 0; i < nb_pending_member_funcs; ++i)
    if (pending_member_funcs[i]
        && pending_member_funcs[i]->func_tok == function_tok
        && pending_member_funcs[i]->auto_return_state) {
      pending = pending_member_funcs[i];
      break;
    }
  if (!pending) return;
  if (pending->auto_return_state == 2)
    cprime_error("use of '%s' before deduction of its auto return type",
                 get_tok_str(function_tok, NULL));
  function = global_symbol_find(function_tok);
  if (!function || !function->type.ref)
    cprime_error("missing pending member declaration");
  pending->auto_return_state = 2;
  prepare_saved_cpp_lambdas(pending->str);
  for (i = 0; i < pending->str->len; ++i) {
    if (pending->str->str[i] == function_tok) { name_at = i; break; }
    if (TOK_HAS_VALUE(pending->str->str[i]))
      i += tok_str_value_extra_words(pending->str->str, pending->str->len, i);
  }
  if (name_at < 0) cprime_error("missing pending member declarator");
  probe = tok_str_alloc();
  add_ctype_tokens(probe, &function->type.ref->type);
  for (i = name_at + 1; i < pending->str->len; ++i)
    tok_str_add_record(probe, pending->str->str, pending->str->len, &i);
  if (!template_probe_type_ex(probe, pending->auto_return_kind == 2 ? 5 : 4,
        symbol_namespace_scope_tok(pending->struct_tok), pending->struct_tok,
        pending->func_tok, &result))
    cprime_error("cannot deduce member return type");
  tok_str_free(probe);
  function->type.ref->type = result;
  member = lookup_cpp_member_decl(function_tok);
  if (member)
    note_cpp_member_decl(member->owner_tok, member->source_name_tok,
                         function_tok, &function->type,
                         pending->is_static_member, !pending->is_static_member);
  definition = tok_str_alloc();
  if (!pending->is_local_class_member) {
    tok_str_add(definition, TOK_STATIC);
    tok_str_add(definition, TOK_INLINE1);
  }
  if (!add_ctype_tokens(definition, &result))
    cprime_error("unsupported deduced member return type");
  for (i = name_at; i < pending->str->len; ++i)
    tok_str_add_record(definition, pending->str->str, pending->str->len, &i);
  tok_str_free(pending->str);
  pending->str = definition;
  pending->auto_return_state = 0;
}

static void add_lifecycle_param_tokens(TokenString *str, TokenString *params,
                                       int struct_tok)
{
  CType return_type = { VT_VOID, NULL };
  CType signature = make_member_func_type_from_saved_params(&return_type,
                                                            params, struct_tok);
  Sym *argument;
  int count = 0;
  /* Re-emit the parsed declarators, preserving parameter names and defaults.
     Text substitution cannot distinguish a nested type from a parameter that
     deliberately hides the same type name. */
  for (argument = signature.ref->next; argument; argument = argument->next)
  {
    if (count++) tok_str_add(str, ',');
    if (!add_ctype_declarator_tokens(str, &argument->type,
                                    argument->v & ~SYM_FIELD))
      cprime_error("unsupported constructor parameter type");
    if (argument->default_arg)
    {
      tok_str_add(str, '=');
      tok_str_append_without_eof(str, argument->default_arg);
    }
  }
  if (signature.ref->f.func_type == FUNC_ELLIPSIS)
  {
    if (count) tok_str_add(str, ',');
    tok_str_add(str, TOK_DOTS);
  }
}

static void add_pending_lifecycle_func(CType *struct_type, int method_tok,
                                       TokenString *params,
                                       TokenString *body, int internal,
                                       int is_defaulted, int exception_spec,
                                       int is_explicit)
{
  int struct_tok, mangled_tok, i;
  int is_local_class_member = local_stack || local_scope;
  CType ret_type, func_type, lowered_type;
  PendingMemberFunc *pm;
  TokenString *str;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("inline constructors/destructors require a named struct/class");
  ret_type.t = VT_VOID;
  ret_type.ref = NULL;
  func_type = make_member_func_type_from_saved_params(&ret_type, params, struct_tok);
  func_type.ref->f.func_cpp_explicit = is_explicit;
  note_lifecycle_exception_spec(struct_type, &func_type, method_tok, exception_spec);
  lowered_type = make_lowered_member_func_type(struct_type, &func_type);
  mangled_tok = make_lifecycle_func_tok_for_type(struct_tok, method_tok,
                                                &lowered_type, 1);
  if (pending_member_func_has_body_tok(mangled_tok))
    return;
  /* Out-of-class definitions do not repeat the explicit specifier. */
  {
    Sym *prior = global_symbol_find(mangled_tok);
    if (prior && prior->type.ref)
      lowered_type.ref->f.func_cpp_explicit |= prior->type.ref->f.func_cpp_explicit;
  }
  note_raw_lifecycle_overload(struct_tok, method_tok, mangled_tok, params);
  note_member_func_overload(struct_tok, method_tok, mangled_tok, &lowered_type);
  {
    Sym *function = external_global_sym(mangled_tok, &lowered_type);
    /* In-class and implicitly generated lifecycle definitions obey the
       inline ODR even when first needed while compiling a caller body.
       Body scheduling (which can depend on the active scope) must not
       turn a header definition into a strong external symbol. */
    if (internal)
    {
      function->a.weak = 1;
      update_storage(function);
    }
  }
  if (is_defaulted)
    note_defaulted_member_func(struct_tok, method_tok, &func_type);

  str = tok_str_alloc();
  if (internal && !is_local_class_member)
  {
    tok_str_add(str, TOK_INLINE1);
  }
  tok_str_add(str, TOK_VOID);
  tok_str_add(str, mangled_tok);
  tok_str_add(str, '(');
  tok_str_add(str, TOK_STRUCT);
  tok_str_add(str, struct_tok);
  tok_str_add(str, '*');
  tok_str_add(str, tok_alloc_const("this"));
  if (params)
  {
    if (params->len > 1)
      tok_str_add(str, ',');
    add_lifecycle_param_tokens(str, params, struct_tok);
  }
  tok_str_add(str, ')');
  append_function_exception_spec(str, &func_type.ref->f);
  for (i = 0; i < body->len && body->str[i] != TOK_EOF; ++i)
  {
    if (TOK_HAS_VALUE(body->str[i]))
    {
      tok_str_add_record(str, body->str, body->len, &i);
      continue;
    }
    tok_str_add(str, body->str[i]);
  }
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  pm->token_filter_valid = 0;
  pm->func_tok = mangled_tok;
  pm->struct_tok = struct_tok;
  pm->is_lifecycle_member = 1;
  pm->is_defaulted_default_constructor = is_defaulted
    && method_tok == TOK_CONSTRUCTOR1 && !func_type.ref->next;
  if (pm->is_defaulted_default_constructor)
    note_struct_member_init_list(struct_tok);
  pm->is_local_class_member = is_local_class_member;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static void add_pending_lifecycle_decl(CType *struct_type, int method_tok,
                                       TokenString *params, int exception_spec,
                                       int is_explicit)
{
  int struct_tok, mangled_tok;
  int is_local_class_member = local_stack || local_scope;
  CType ret_type, func_type, lowered_type;
  PendingMemberFunc *pm;
  TokenString *str;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("constructors/destructors require a named struct/class");
  ret_type.t = VT_VOID;
  ret_type.ref = NULL;
  func_type = make_member_func_type_from_saved_params(&ret_type, params, struct_tok);
  func_type.ref->f.func_cpp_explicit = is_explicit;
  note_lifecycle_exception_spec(struct_type, &func_type, method_tok, exception_spec);
  lowered_type = make_lowered_member_func_type(struct_type, &func_type);
  mangled_tok = make_lifecycle_func_tok_for_type(struct_tok, method_tok,
                                                 &lowered_type, 0);
  if (pending_member_func_has_tok(mangled_tok))
    return;
  if (sym_find(mangled_tok) || global_symbol_find(mangled_tok))
    return;
  note_raw_lifecycle_overload(struct_tok, method_tok, mangled_tok, params);
  note_member_func_overload(struct_tok, method_tok, mangled_tok,
                            &lowered_type);
  external_global_sym(mangled_tok, &lowered_type);

  str = tok_str_alloc();
  tok_str_add(str, TOK_VOID);
  tok_str_add(str, mangled_tok);
  tok_str_add(str, '(');
  tok_str_add(str, TOK_STRUCT);
  tok_str_add(str, struct_tok);
  tok_str_add(str, '*');
  tok_str_add(str, tok_alloc_const("this"));
  if (params)
  {
    if (params->len > 1)
      tok_str_add(str, ',');
    add_lifecycle_param_tokens(str, params, struct_tok);
  }
  tok_str_add(str, ')');
  append_function_exception_spec(str, &func_type.ref->f);
  tok_str_add(str, ';');
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  pm->token_filter_valid = 0;
  pm->struct_tok = struct_tok;
  pm->is_local_class_member = is_local_class_member;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static int type_requires_class_construction(CType *type)
{
  Sym *field;
  VirtualTableInfo *table;
  if ((type->t & VT_ARRAY) && type->ref)
    return type_requires_class_construction(&type->ref->type);
  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref) return 0;
  if (type->ref->a.lifecycle_ctor) return 1;
  for (table = virtual_table_infos; table; table = table->next)
    if (table->class_tok == get_struct_type_name_tok(type)) return 1;
  if (IS_UNION(type->t)) return 0;
  for (field = type->ref->next; field; field = field->next)
    if (field->default_arg || type_requires_class_construction(&field->type)) return 1;
  return 0;
}

static TemplateDef *find_template_def(int name_tok);

static int split_saved_array_initializer(TokenString *initializer,
                                          TokenString ***elements)
{
  int i, level = 0, angle = 0, previous = 0, count = 0, have_value = 0;
  TokenString *element;
  *elements = NULL;
  if (!initializer) return 0;
  i = tok_str_next_non_linenum_index(initializer->str, initializer->len, 0);
  if (i >= initializer->len || initializer->str[i] == TOK_EOF) return 0;
  if (initializer->str[i] != '{')
    cprime_error("array initializer requires braces");
  element = tok_str_alloc();
  for (++i; i < initializer->len; ++i)
  {
    int token = initializer->str[i];
    if (TOK_HAS_VALUE(token))
    {
      tok_str_add_record(element, initializer->str, initializer->len, &i);
      if (token != TOK_LINENUM) have_value = 1;
      continue;
    }
    if (!level && !angle && (token == ',' || token == '}'))
    {
      if (have_value)
      {
        tok_str_add(element, TOK_EOF);
        dynarray_add(elements, &count, element);
        element = tok_str_alloc();
        have_value = 0;
      }
      else if (token == ',')
        cprime_error("array initializer element expected");
      if (token == '}') break;
      previous = 0;
      continue;
    }
    if (token == TOK_EOF) cprime_error("unterminated array initializer");
    tok_str_add(element, token);
    have_value = 1;
    if (token == '(' || token == '[' || token == '{' || is_cpp_lambda_marker(token))
      ++level;
    else if (token == ')' || token == ']' || token == '}')
      --level;
    else if ((token == TOK_LT || token == '<')
             && (angle || (previous >= TOK_UIDENT && find_template_def(previous))))
      ++angle;
    else if (token == TOK_GT && angle) --angle;
    else if ((token == TOK_SAR || token == TOK_SHR) && angle)
      angle = angle > 1 ? angle - 2 : 0;
    previous = token;
  }
  tok_str_free(element);
  return count;
}

static void free_saved_array_elements(TokenString **elements, int count)
{
  int i;
  for (i = 0; i < count; ++i) tok_str_free(elements[i]);
  cprime_free(elements);
}

static int saved_initializer_starts_brace(TokenString *initializer)
{
  int first = tok_str_next_non_linenum_index(initializer->str, initializer->len, 0);
  return first < initializer->len && initializer->str[first] == '{';
}

static void consume_global_array_element(CType *type, TokenString **elements,
                                          int count, int *index)
{
  int i;
  if (*index >= count) return;
  if (!(type->t & VT_ARRAY) || saved_initializer_starts_brace(elements[*index]))
  {
    ++*index;
    return;
  }
  if (!type->ref || type->ref->c <= 0)
    cprime_error("incomplete inner array bound");
  for (i = 0; i < type->ref->c && *index < count; ++i)
    consume_global_array_element(&type->ref->type, elements, count, index);
}

static void complete_global_class_array_bound(CType *type, TokenString *initializer)
{
  TokenString **elements;
  int count, index = 0, bound = 0;
  if (!(type->t & VT_ARRAY) || !type->ref || type->ref->c >= 0) return;
  count = split_saved_array_initializer(initializer, &elements);
  if (!count) cprime_error("cannot infer an empty array bound");
  while (index < count)
  {
    consume_global_array_element(&type->ref->type, elements, count, &index);
    ++bound;
  }
  type->ref->c = bound;
  free_saved_array_elements(elements, count);
}

static int can_lower_global_dynamic_init(CType *type, int l, int has_init)
{
  int bt;

  if (!is_cpp_translation_unit() || l != VT_CONST || !has_init)
    return 0;
  if (type_requires_class_construction(type))
    return 1;
  if (type->t & VT_ARRAY)
    return 0;
  bt = type->t & VT_BTYPE;
  if (bt == VT_STRUCT)
    return get_struct_type_name_tok(type) && type->ref
           && type->ref->a.lifecycle_ctor;
  return bt != VT_FUNC && bt != VT_VOID;
}

static int type_requires_class_destruction(CType *type);

static int can_lower_local_static_dynamic_init(CType *type, int l,
                                               int has_init)
{
  CType *element = type;
  while ((element->t & VT_ARRAY) && element->ref)
    element = &element->ref->type;
  if (!is_cpp_translation_unit() || l == VT_CONST || !(type->t & VT_STATIC))
    return 0;
  if (!(type->t & VT_ARRAY) && (element->t & VT_BTYPE) != VT_STRUCT)
    return has_init && (element->t & VT_BTYPE) != VT_FUNC
                    && (element->t & VT_BTYPE) != VT_VOID;
  return (element->t & VT_BTYPE) == VT_STRUCT
         && get_struct_type_name_tok(element) && element->ref
         && (has_init || type_requires_class_construction(type)
             || type_requires_class_destruction(type));
}

static void append_global_class_initialization(TokenString *body, CType *type,
                                                TokenString *path,
                                                TokenString *initializer);
static void append_counted_class_initialization(TokenString *body, CType *type,
                                                TokenString *path,
                                                TokenString *initializer,
                                                int counter_tok);

static int static_runtime_helper(const char *name, int registration, int acquire)
{
  int helper = tok_alloc_const(name);
  if (!global_symbol_find(helper))
  {
    CType result = { acquire ? VT_INT : VT_VOID, NULL }, function;
    TokenString *parameters = tok_str_alloc();
    tok_str_add(parameters, TOK_VOID);
    tok_str_add(parameters, '*');
    if (registration)
    {
      tok_str_add(parameters, ',');
      tok_str_add(parameters, TOK_VOID);
      tok_str_add(parameters, '(');
      tok_str_add(parameters, '*');
      tok_str_add(parameters, ')');
      tok_str_add(parameters, '(');
      tok_str_add(parameters, TOK_VOID);
      tok_str_add(parameters, '*');
      tok_str_add(parameters, ')');
      tok_str_add(parameters, ',');
      tok_str_add(parameters, TOK_UNSIGNED);
      tok_str_add(parameters, TOK_LONG);
      tok_str_add(parameters, TOK_LONG);
      tok_str_add(parameters, ',');
      tok_str_add(parameters, TOK_UNSIGNED);
      tok_str_add(parameters, TOK_LONG);
      tok_str_add(parameters, TOK_LONG);
    }
    tok_str_add(parameters, TOK_EOF);
    function = make_func_type_from_saved_params(&result, parameters);
    external_global_sym(helper, &function);
    tok_str_free(parameters);
  }
  return helper;
}

static void append_static_destructor_operation(TokenString *body, int var_tok,
                                                CType *type, const char *name,
                                                int counter_tok)
{
  CType element = *type;
  Sym *destructor;
  CValue value;
  unsigned long long count = 1;
  int alignment, helper;
  while ((element.t & VT_ARRAY) && element.ref)
  {
    count *= element.ref->c;
    element = element.ref->type;
  }
  destructor = resolve_autodtor_func(&element);
  if (!destructor) return;
  helper = static_runtime_helper(name, 1, 0);
  tok_str_add(body, helper);
  tok_str_add(body, '(');
  tok_str_add(body, '(');
  tok_str_add(body, TOK_VOID);
  tok_str_add(body, '*');
  tok_str_add(body, ')');
  tok_str_add(body, '&');
  tok_str_add(body, var_tok);
  tok_str_add(body, ',');
  tok_str_add(body, '(');
  tok_str_add(body, TOK_VOID);
  tok_str_add(body, '(');
  tok_str_add(body, '*');
  tok_str_add(body, ')');
  tok_str_add(body, '(');
  tok_str_add(body, TOK_VOID);
  tok_str_add(body, '*');
  tok_str_add(body, ')');
  tok_str_add(body, ')');
  tok_str_add(body, destructor->v);
  tok_str_add(body, ',');
  if (counter_tok)
    tok_str_add(body, counter_tok);
  else
  {
    value.i = count;
    tok_str_add2(body, TOK_CULLONG, &value);
  }
  tok_str_add(body, ',');
  value.i = type_size(&element, &alignment);
  tok_str_add2(body, TOK_CULLONG, &value);
  tok_str_add(body, ')');
  tok_str_add(body, ';');
}

static void append_static_destructor_registration(TokenString *body, int var_tok,
                                                   CType *type)
{
  append_static_destructor_operation(body, var_tok, type,
                                      "__cpc_register_static_destructor", 0);
}

static void append_static_guard_call(TokenString *body, const char *name,
                                     int guard_tok)
{
  int helper = static_runtime_helper(name, 0,
                                      !strcmp(name, "__cpc_static_acquire"));
  tok_str_add(body, helper);
  tok_str_add(body, '(');
  tok_str_add(body, '&');
  tok_str_add(body, guard_tok);
  tok_str_add(body, ')');
}

static void emit_local_static_dynamic_init(int var_tok, CType *type,
                                           TokenString *init)
{
  static int local_static_guard_counter;
  char name[64];
  int guard_tok, counter_tok = 0;
  int saved_tok = tok;
  CValue saved_tokc = tokc;
  TokenString *saved_macro_stack = macro_stack;
  const int *saved_macro_ptr = macro_ptr;
  TokenString *str, *path;

  snprintf(name, sizeof(name), "__cpc_local_static_guard_%d",
           ++local_static_guard_counter);
  guard_tok = tok_alloc_const(name);
  if (type->t & VT_ARRAY)
  {
    snprintf(name, sizeof(name), "__cpc_local_static_count_%d",
             local_static_guard_counter);
    counter_tok = tok_alloc_const(name);
  }

  /* Construction happens at the declaration. Publish completion only after
     construction succeeds, and reset the guard if an exception escapes. */
  str = tok_str_alloc();
  tok_str_add(str, '{');
  tok_str_add(str, TOK_STATIC);
  tok_str_add(str, TOK_INT);
  tok_str_add(str, guard_tok);
  tok_str_add(str, ';');
  tok_str_add(str, TOK_IF);
  tok_str_add(str, '(');
  append_static_guard_call(str, "__cpc_static_acquire", guard_tok);
  tok_str_add(str, ')');
  tok_str_add(str, '{');
  if (counter_tok)
  {
    CValue zero = {0};
    tok_str_add(str, TOK_UNSIGNED);
    tok_str_add(str, TOK_LONG);
    tok_str_add(str, TOK_LONG);
    tok_str_add(str, counter_tok);
    tok_str_add(str, '=');
    tok_str_add2(str, TOK_CINT, &zero);
    tok_str_add(str, ';');
  }
  tok_str_add(str, tok_alloc_const("try"));
  tok_str_add(str, '{');
  path = tok_str_alloc();
  tok_str_add(path, var_tok);
  if ((type->t & VT_BTYPE) == VT_STRUCT || (type->t & VT_ARRAY))
    append_counted_class_initialization(str, type, path, init, counter_tok);
  else {
    tok_str_append_without_eof(str, path);
    tok_str_add(str, TOK_INIT_MEMBER);
    tok_str_append_without_eof(str, init);
    tok_str_add(str, ';');
  }
  tok_str_free(path);
  append_static_destructor_registration(str, var_tok, type);
  append_static_guard_call(str, "__cpc_static_release", guard_tok);
  tok_str_add(str, ';');
  tok_str_add(str, '}');
  tok_str_add(str, tok_alloc_const("catch"));
  tok_str_add(str, '(');
  tok_str_add(str, TOK_DOTS);
  tok_str_add(str, ')');
  tok_str_add(str, '{');
  if (counter_tok)
    append_static_destructor_operation(str, var_tok, type,
                                       "__cpc_static_destroy_elements", counter_tok);
  append_static_guard_call(str, "__cpc_static_abort", guard_tok);
  tok_str_add(str, ';');
  tok_str_add(str, tok_alloc_const("throw"));
  tok_str_add(str, ';');
  tok_str_add(str, '}');
  tok_str_add(str, '}');
  tok_str_add(str, '}');
  tok_str_add(str, TOK_EOF);

  begin_macro(str, 1);
  next();
  block(STMT_COMPOUND);
  while (macro_stack && macro_stack != saved_macro_stack)
    end_macro();
  macro_ptr = saved_macro_ptr;
  tok = saved_tok;
  tokc = saved_tokc;
}

static Sym *resolve_initializer_list_constructor(CType *type)
{
  int struct_tok = get_struct_type_name_tok(type);
  MemberFuncOverload *o;
  TemplateDef *td;
  int i, j;

  if (!struct_tok)
    return NULL;
  /* Class-template lifecycle declarations may still be queued until their
     first call.  Force the one-argument constructor set to be materialized
     before inspecting its parameter types. */
  (void)resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1, 1);
  /* Out-of-class class-template constructors with a std::initializer_list
     parameter are captured as template members and only become concrete
     overloads once instantiated.  A braced initializer always targets the
     initializer-list constructor, so materialize it for this class before
     scanning the overload table. */
  td = find_class_template_def_for_class_tok(struct_tok);
  if (td)
    for (j = 0; j < td->nb_inst; ++j)
      if (td->inst_name_toks[j] == struct_tok)
        for (i = 0; i < nb_template_member_defs; ++i)
        {
          TemplateMemberDef *md = template_member_defs[i];
          if (md->class_tok == td->name_tok
              && template_member_def_method_tok(md) == TOK_CONSTRUCTOR1
              && token_string_has_initializer_list(md->def_str))
            instantiate_template_member_body_if_needed(
                md, td->inst_type_toks[j * td->nb_type_params],
                struct_tok, 0);
        }
  for (o = member_func_overloads; o; o = o->next)
  {
    Sym *param, *s;
    if (profile_scans_enabled) ++profile_other_overload_scans[6];
    CType *param_type;

    if (o->struct_tok != struct_tok || o->method_tok != TOK_CONSTRUCTOR1
        || o->explicit_arg_count != 1)
      continue;
    s = global_symbol_find(o->mangled_tok);
    if (!s)
      s = sym_find(o->mangled_tok);
    if (!s)
      continue;
    use_overload_func_type(s, &o->func_type);
    param = s->type.ref ? s->type.ref->next : NULL;
    if (param)
      param = param->next; /* skip the lowered this parameter */
    if (!param)
      continue;
    param_type = &param->type;
    if (is_reference_type(param_type))
      param_type = pointed_type(param_type);
    if (!type_is_std_initializer_list(param_type))
      continue;
    instantiate_template_member_body_for_func_tok(type, TOK_CONSTRUCTOR1,
                                                  s->v);
    return s;
  }
  return NULL;
}

static int class_has_initializer_list_constructor_candidate(CType *type)
{
  int struct_tok = get_struct_type_name_tok(type);
  MemberFuncOverload *o;
  TemplateDef *td;
  int i, j;

  if (!struct_tok)
    return 0;
  if (resolve_initializer_list_constructor(type))
    return 1;
  for (o = member_func_overloads; o; o = o->next)
  {
    Sym *param;
    if (profile_scans_enabled) ++profile_other_overload_scans[7];
    CType *param_type;

    if (o->struct_tok != struct_tok || o->method_tok != TOK_CONSTRUCTOR1
        || o->explicit_arg_count != 1 || !o->func_type.ref)
      continue;
    param = o->func_type.ref->next;
    if (param)
      param = param->next; /* skip lowered this */
    if (!param)
      continue;
    param_type = &param->type;
    if (is_reference_type(param_type))
      param_type = pointed_type(param_type);
    if (type_is_std_initializer_list(param_type))
      return 1;
  }

  td = find_class_template_def_for_class_tok(struct_tok);
  if (!td)
  {
    const char *struct_name = get_tok_str(struct_tok, NULL);
    for (i = 0; i < nb_template_defs; ++i)
    {
      const char *base_name;
      size_t base_len;
      td = template_defs[i];
      if (!td->is_class)
      {
        td = NULL;
        continue;
      }
      base_name = get_tok_str(td->lookup_tok ? td->lookup_tok : td->name_tok,
                              NULL);
      base_len = strlen(base_name);
      if (!strncmp(struct_name, base_name, base_len)
          && struct_name[base_len] == '_'
          && struct_name[base_len + 1] == '_')
        break;
      td = NULL;
    }
  }
  if (!td)
    return 0;
  if (td->def_str && token_string_has_initializer_list(td->def_str))
    return 1;
  for (j = 0; j < td->nb_inst; ++j)
    if (td->inst_name_toks[j] == struct_tok)
      for (i = 0; i < nb_template_member_defs; ++i)
      {
        TemplateMemberDef *md = template_member_defs[i];
        if (md->class_tok == td->name_tok
            && template_member_def_method_tok(md) == TOK_CONSTRUCTOR1
            && token_string_has_initializer_list(md->def_str))
          return 1;
      }
  return 0;
}

static int append_global_array_index(TokenString *path, int index, int is_token)
{
  int old_length = path->len;
  CValue value;
  tok_str_add(path, '[');
  if (is_token) tok_str_add(path, index);
  else
  {
    value.i = index;
    tok_str_add2(path, TOK_CINT, &value);
  }
  tok_str_add(path, ']');
  return old_length;
}

static int saved_initializer_last_token(TokenString *initializer)
{
  int i, last = -1;
  for (i = 0; i < initializer->len && initializer->str[i] != TOK_EOF; ++i)
  {
    int token = initializer->str[i];
    if (token != TOK_LINENUM) last = i;
    if (TOK_HAS_VALUE(token))
      i += tok_str_value_extra_words(initializer->str, initializer->len, i);
  }
  return last;
}

static void append_counted_class_initialization(TokenString *body, CType *type,
                                                TokenString *path,
                                                TokenString *initializer,
                                                int counter_tok)
{
  if (type_is_std_initializer_list(type) && initializer
      && saved_initializer_starts_brace(initializer))
  {
    tok_str_append_without_eof(body, path);
    tok_str_add(body, TOK_INIT_MEMBER);
    tok_str_append_without_eof(body, initializer);
    tok_str_add(body, ';');
    return;
  }
  if (type->t & VT_ARRAY)
  {
    TokenString **elements;
    int count = split_saved_array_initializer(initializer, &elements);
    int i = 0, input_index = 0, old_length;
    if (!type->ref || type->ref->c < 0)
      cprime_error("global class array has incomplete bounds");
    while (i < type->ref->c && input_index < count)
    {
      TokenString *element = elements[input_index], *grouped = NULL;
      int start = input_index, j;
      consume_global_array_element(&type->ref->type, elements, count, &input_index);
      if ((type->ref->type.t & VT_ARRAY) && !saved_initializer_starts_brace(element))
      {
        /* Brace elision fills the current subarray before advancing to its
           sibling. Preserve those elements as one nested initializer. */
        grouped = tok_str_alloc();
        tok_str_add(grouped, '{');
        for (j = start; j < input_index; ++j)
        {
          if (j != start) tok_str_add(grouped, ',');
          tok_str_append_without_eof(grouped, elements[j]);
        }
        tok_str_add(grouped, '}');
        tok_str_add(grouped, TOK_EOF);
        element = grouped;
      }
      old_length = append_global_array_index(path, i, 0);
      append_counted_class_initialization(body, &type->ref->type, path, element,
                                           counter_tok);
      path->len = old_length;
      if (grouped) tok_str_free(grouped);
      ++i;
    }
    if (input_index < count)
      cprime_error("too many initializers for class array");
    if (i < type->ref->c)
    {
      char name[64];
      int index;
      CValue value;
      snprintf(name, sizeof(name), "__cpc_global_array_index_%u", anon_sym++);
      index = tok_alloc_const(name);
      tok_str_add(body, TOK_FOR);
      tok_str_add(body, '(');
      tok_str_add(body, TOK_INT);
      tok_str_add(body, index);
      tok_str_add(body, '=');
      value.i = i;
      tok_str_add2(body, TOK_CINT, &value);
      tok_str_add(body, ';');
      tok_str_add(body, index);
      tok_str_add(body, TOK_LT);
      value.i = type->ref->c;
      tok_str_add2(body, TOK_CINT, &value);
      tok_str_add(body, ';');
      tok_str_add(body, TOK_INC);
      tok_str_add(body, index);
      tok_str_add(body, ')');
      tok_str_add(body, '{');
      old_length = append_global_array_index(path, index, 1);
      append_counted_class_initialization(body, &type->ref->type, path, NULL,
                                           counter_tok);
      path->len = old_length;
      tok_str_add(body, '}');
    }
    free_saved_array_elements(elements, count);
  }
  else
  {
    int first = 0, end = initializer ? initializer->len : 0, i;
    int struct_tok = get_struct_type_name_tok(type);
    if (!struct_tok) cprime_error("class initializer requires a named class");
    if (initializer)
    {
      first = tok_str_next_non_linenum_index(initializer->str, initializer->len, 0);
      end = saved_initializer_last_token(initializer);
      if (first <= end && initializer->str[first] == '{'
          && type->ref && type->ref->a.lifecycle_ctor
          && !class_has_initializer_list_constructor_candidate(type))
      {
        ++first;
        --end;
      }
    }
    else end = -1;
    tok_str_add(body, tok_alloc_const("new"));
    tok_str_add(body, '(');
    tok_str_add(body, '&');
    tok_str_append_without_eof(body, path);
    tok_str_add(body, ')');
    tok_str_add(body, TOK_STRUCT);
    tok_str_add(body, struct_tok);
    tok_str_add(body, '(');
    for (i = first; i <= end; ++i)
    {
      if (TOK_HAS_VALUE(initializer->str[i]))
        tok_str_add_record(body, initializer->str, initializer->len, &i);
      else tok_str_add(body, initializer->str[i]);
    }
    tok_str_add(body, ')');
    tok_str_add(body, ';');
    if (counter_tok)
    {
      tok_str_add(body, TOK_INC);
      tok_str_add(body, counter_tok);
      tok_str_add(body, ';');
    }
  }
}

static void append_global_class_initialization(TokenString *body, CType *type,
                                                TokenString *path,
                                                TokenString *initializer)
{
  append_counted_class_initialization(body, type, path, initializer, 0);
}

static int type_requires_class_destruction(CType *type)
{
  Sym *field;
  if (type->t & VT_ARRAY)
    return type->ref && type_requires_class_destruction(&type->ref->type);
  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref) return 0;
  if (type->ref->a.lifecycle_dtor) return 1;
  for (field = type->ref->next; field; field = field->next)
    if (type_requires_class_destruction(&field->type)) return 1;
  return 0;
}

static void add_pending_global_dynamic_init(int var_tok, CType *type,
                                            TokenString *init)
{
  char name[64];
  int func_tok, i;
  PendingMemberFunc *pm;
  TokenString *str;

  snprintf(name, sizeof(name), "__cpc_global_dynamic_init_%d",
           ++nb_pending_global_inits);
  func_tok = tok_alloc_const(name);

  str = tok_str_alloc();
  tok_str_add(str, TOK_STATIC);
  tok_str_add(str, TOK_VOID);
  tok_str_add(str, func_tok);
  tok_str_add(str, '(');
  tok_str_add(str, ')');
  tok_str_add(str, TOK_ATTRIBUTE1);
  tok_str_add(str, '(');
  tok_str_add(str, '(');
  tok_str_add(str, TOK_CONSTRUCTOR1);
  tok_str_add(str, ')');
  tok_str_add(str, ')');
  tok_str_add(str, '{');
  if (type_requires_class_construction(type))
  {
    TokenString *path = tok_str_alloc();
    tok_str_add(path, var_tok);
    append_global_class_initialization(str, type, path, init);
    tok_str_free(path);
    goto close_initializer;
  }
  if ((type->t & VT_BTYPE) == VT_STRUCT)
  {
    int struct_tok = get_struct_type_name_tok(type);
    if (!struct_tok)
      cprime_error("global class initializer requires a named class");
    tok_str_add(str, tok_alloc_const("new"));
    tok_str_add(str, '(');
    tok_str_add(str, '&');
    tok_str_add(str, var_tok);
    tok_str_add(str, ')');
    tok_str_add(str, TOK_STRUCT);
    tok_str_add(str, struct_tok);
    tok_str_add(str, '(');
  }
  else
  {
    tok_str_add(str, var_tok);
    tok_str_add(str, TOK_INIT_MEMBER);
  }
  for (i = 0; i < init->len && init->str[i] != TOK_EOF; ++i)
  {
    if (TOK_HAS_VALUE(init->str[i]))
    {
      tok_str_add_record(str, init->str, init->len, &i);
      continue;
    }
    tok_str_add(str, init->str[i]);
  }
  if ((type->t & VT_BTYPE) == VT_STRUCT)
    tok_str_add(str, ')');
  tok_str_add(str, ';');
close_initializer:
  append_static_destructor_registration(str, var_tok, type);
  tok_str_add(str, '}');
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  pm->token_filter_valid = 0;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static int pending_lifecycle_is_constructor(PendingMemberFunc *pm)
{
  int i;

  if (!pm || !pm->is_lifecycle_member || !pm->str)
    return 0;
  for (i = 0; i < pm->str->len && pm->str->str[i] != '{'; ++i)
  {
    int t = pm->str->str[i];
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(pm->str->str, pm->str->len, i);
      continue;
    }
    if (t >= TOK_UIDENT && strstr(get_tok_str(t, NULL), "constructor"))
      return 1;
  }
  return 0;
}

static void append_saved_initializer_tokens(TokenString *dst,
                                            TokenString *initializer)
{
  int i;

  if (!initializer)
    return;
  for (i = 0; i < initializer->len
              && initializer->str[i] != TOK_EOF; ++i)
  {
    if (TOK_HAS_VALUE(initializer->str[i]))
      tok_str_add_record(dst, initializer->str, initializer->len, &i);
    else
      tok_str_add(dst, initializer->str[i]);
  }
}

static void append_omitted_constructor_member_initializer(TokenString *dst,
                                                          Sym *field,
                                                          int this_tok)
{
  int field_tok = field->v & ~SYM_FIELD;
  int field_struct_tok = get_struct_type_name_tok(&field->type);

  if ((field->type.t & VT_ARRAY) && field->default_arg)
  {
    tok_str_add(dst, this_tok);
    tok_str_add(dst, TOK_ARROW);
    tok_str_add(dst, field_tok);
    tok_str_add(dst, TOK_INIT_MEMBER);
    append_saved_initializer_tokens(dst, field->default_arg);
    tok_str_add(dst, ';');
    return;
  }

  if ((field->type.t & VT_BTYPE) == VT_STRUCT && field_struct_tok)
  {
    tok_str_add(dst, tok_alloc_const("new"));
    tok_str_add(dst, '(');
    tok_str_add(dst, '&');
    tok_str_add(dst, '(');
    tok_str_add(dst, this_tok);
    tok_str_add(dst, TOK_ARROW);
    tok_str_add(dst, field_tok);
    tok_str_add(dst, ')');
    tok_str_add(dst, ')');
    tok_str_add(dst, TOK_STRUCT);
    tok_str_add(dst, field_struct_tok);
    tok_str_add(dst, '(');
    append_saved_initializer_tokens(dst, field->default_arg);
    tok_str_add(dst, ')');
    tok_str_add(dst, ';');
    return;
  }

  if (field->default_arg)
  {
    tok_str_add(dst, this_tok);
    tok_str_add(dst, TOK_ARROW);
    tok_str_add(dst, field_tok);
    tok_str_add(dst, TOK_INIT_MEMBER);
    append_saved_initializer_tokens(dst, field->default_arg);
    tok_str_add(dst, ';');
  }
}

static void inject_omitted_constructor_member_initializers(
  PendingMemberFunc *pm, CType *class_type, int this_tok)
{
  TokenString *initialized, *dst;
  Sym *field;
  int body_open = -1, prefix_end, i, insert_at, delegating = 0;
  int new_tok = tok_alloc_const("new");

  if (!pending_lifecycle_is_constructor(pm)
      || !struct_has_member_init_list(pm->struct_tok))
    return;

  for (i = 0; i < pm->str->len; ++i)
  {
    int t = pm->str->str[i];
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(pm->str->str, pm->str->len, i);
      continue;
    }
    if (t == '{')
    {
      body_open = i;
      break;
    }
  }
  if (body_open < 0)
    return;

  initialized = tok_str_alloc();
  prefix_end = body_open + 1;
  for (;;)
  {
    int field_tok = 0, statement_end;

    if (prefix_end + 3 < pm->str->len
        && pm->str->str[prefix_end] == this_tok
        && pm->str->str[prefix_end + 1] == TOK_ARROW
        && pm->str->str[prefix_end + 2] >= TOK_UIDENT
        && (pm->str->str[prefix_end + 3] == '='
            || pm->str->str[prefix_end + 3] == TOK_INIT_MEMBER))
      field_tok = pm->str->str[prefix_end + 2];
    else if (prefix_end + 8 < pm->str->len
             && pm->str->str[prefix_end] == new_tok
             && pm->str->str[prefix_end + 1] == '('
             && pm->str->str[prefix_end + 2] == '&'
             && pm->str->str[prefix_end + 3] == '('
             && pm->str->str[prefix_end + 4] == this_tok
             && pm->str->str[prefix_end + 5] == TOK_ARROW
             && pm->str->str[prefix_end + 6] >= TOK_UIDENT)
      field_tok = pm->str->str[prefix_end + 6];
    else if (prefix_end + 3 < pm->str->len
             && pm->str->str[prefix_end] == ':'
             && pm->str->str[prefix_end + 1] == ':'
             && pm->str->str[prefix_end + 2] >= TOK_UIDENT
             && strstr(get_tok_str(pm->str->str[prefix_end + 2], NULL),
                       "constructor"))
      delegating = 1;
    else if (prefix_end < pm->str->len
             && pm->str->str[prefix_end] == TOK_CXX_EH_CONSTRUCTED)
    {
      /* Completion metadata belongs to the initializer just scanned. Keep
         walking so later explicit/default initializers are not reinserted
         ahead of the cleanup activation for that subobject. */
    }
    else
      break;

    statement_end = prefix_end;
    while (statement_end < pm->str->len
           && pm->str->str[statement_end] != ';'
           && pm->str->str[statement_end] != TOK_EOF)
    {
      if (TOK_HAS_VALUE(pm->str->str[statement_end]))
        statement_end += 1 + tok_str_value_extra_words(
          pm->str->str, pm->str->len, statement_end);
      else
        ++statement_end;
    }
    if (statement_end >= pm->str->len
        || pm->str->str[statement_end] != ';')
      break;
    if (field_tok)
      tok_str_add(initialized, field_tok);
    prefix_end = statement_end + 1;
    if (delegating)
      break;
  }

  if (delegating)
  {
    tok_str_free(initialized);
    return;
  }

  dst = tok_str_alloc();
  insert_at = prefix_end;
  for (i = 0; i < pm->str->len && pm->str->str[i] != TOK_EOF; ++i)
  {
    if (i == insert_at)
      for (field = class_type->ref ? class_type->ref->next : NULL;
           field; field = field->next)
      {
        int field_tok = field->v & ~SYM_FIELD;
        if (field_tok < TOK_UIDENT || field_tok >= SYM_FIRST_ANOM
            || constructor_field_was_explicitly_initialized(initialized,
                                                            field_tok))
          continue;
        append_omitted_constructor_member_initializer(dst, field, this_tok);
      }
    if (TOK_HAS_VALUE(pm->str->str[i]))
      tok_str_add_record(dst, pm->str->str, pm->str->len, &i);
    else
      tok_str_add(dst, pm->str->str[i]);
  }
  tok_str_add(dst, TOK_EOF);
  tok_str_free(initialized);
  tok_str_free(pm->str);
  pm->str = dst;
  pm->token_filter_valid = 0;
}

static int pending_member_signature_has_param_name(TokenString *str,
                                                   int name_tok)
{
  int i, level = 0, in_params = 0;

  if (!str || name_tok < TOK_UIDENT)
    return 0;
  for (i = 0; i < str->len && str->str[i] != TOK_EOF; ++i)
  {
    int t = str->str[i];
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(str->str, str->len, i);
      continue;
    }
    if (!in_params)
    {
      if (t == '(')
      {
        in_params = 1;
        level = 1;
      }
      continue;
    }
    if (t == '(' || t == '[' || t == TOK_LT)
    {
      ++level;
      continue;
    }
    if (t == ')' || t == ']' || t == TOK_GT)
    {
      --level;
      if (level == 0)
        return 0;
      continue;
    }
    if ((t & ~SYM_FIELD) == (name_tok & ~SYM_FIELD))
    {
      int next_i = tok_str_next_non_linenum_index(str->str, str->len, i + 1);
      if (next_i < str->len
          && (str->str[next_i] == ',' || str->str[next_i] == ')'
              || str->str[next_i] == '=' || str->str[next_i] == '['))
        return 1;
    }
  }
  return 0;
}

static void complete_defaulted_constructor_body(PendingMemberFunc *pm,
                                                CType *class_type)
{
  TokenString *body, *initializers;
  int i, saved_tok = tok;
  CValue saved_tokc = tokc;

  if (!pm->is_defaulted_default_constructor)
    return;
  /* A defaulted constructor can be declared before every data member. Its
     body must be formed from the completed class, independently of which
     other constructors happened to be instantiated in this translation
     unit. Otherwise inline ODR coalescing can select an empty definition. */
  body = tok_str_alloc();
  for (i = 0; i < pm->str->len; ++i)
  {
    if (TOK_HAS_VALUE(pm->str->str[i]))
      tok_str_add_record(body, pm->str->str, pm->str->len, &i);
    else
    {
      tok_str_add(body, pm->str->str[i]);
      if (pm->str->str[i] == '{')
        break;
    }
  }
  tok = '{';
  initializers = parse_constructor_member_initializers(class_type);
  tok = saved_tok;
  tokc = saved_tokc;
  if (initializers)
  {
    tok_str_append(body, initializers);
    tok_str_free(initializers);
  }
  tok_str_add(body, '}');
  tok_str_add(body, TOK_EOF);
  tok_str_free(pm->str);
  pm->str = body;
  pm->token_filter_valid = 0;
}

static void rewrite_pending_member_field_access(PendingMemberFunc *pm)
{
  int i, in_body = 0, this_tok;
  Sym *class_sym;
  CType class_type;
  TokenString *dst;

  if (!pm || !pm->struct_tok || !pm->str || pm->is_static_member)
    return;
  class_sym = struct_find(pm->struct_tok);
  if (!class_sym)
    return;
  class_type = class_sym->type;
  class_type.ref = class_sym;
  complete_defaulted_constructor_body(pm, &class_type);
  this_tok = 0;
  for (i = 0; i < pm->str->len; ++i)
  {
    int t = pm->str->str[i];
    if (TOK_HAS_VALUE(t))
    {
      i += tok_str_value_extra_words(pm->str->str, pm->str->len, i);
      continue;
    }
    if (t >= TOK_UIDENT
        && !strncmp(get_tok_str(t, NULL), "__cprime_this_", 14))
    {
      this_tok = t;
      break;
    }
  }
  if (!this_tok)
    this_tok = tok_alloc_const("this");
  if (!pm->is_defaulted_default_constructor)
    inject_omitted_constructor_member_initializers(pm, &class_type, this_tok);
  prepare_saved_cpp_lambdas(pm->str);
  pm->token_filter_valid = 0;
  dst = tok_str_alloc();
  for (i = 0; i < pm->str->len && pm->str->str[i] != TOK_EOF; ++i)
  {
    int t = pm->str->str[i];
    if (TOK_HAS_VALUE(t))
    {
      tok_str_add_record(dst, pm->str->str, pm->str->len, &i);
      continue;
    }
    if (!in_body)
    {
      tok_str_add(dst, t);
      if (t == '{')
        in_body = 1;
      continue;
    }
    if (append_nested_lambda_tokens(dst, pm->str, &i))
      continue;
    if (t == this_tok && i + 3 < pm->str->len
        && pm->str->str[i + 1] == TOK_ARROW
        && pm->str->str[i + 2] >= TOK_UIDENT
        && pm->str->str[i + 3] == TOK_INIT_MEMBER)
    {
      int field_tok = pm->str->str[i + 2], dummy_ofs, field_struct_tok, j;
      int rhs_i = i + 4;
      Sym *field = find_field_try(&class_type, field_tok, &dummy_ofs);
      field_struct_tok = field ? get_struct_type_name_tok(&field->type) : 0;
      while (rhs_i + 1 < pm->str->len
             && pm->str->str[rhs_i] == TOK_LINENUM)
        rhs_i += 2;
      if (field && (field->type.t & VT_BTYPE) != VT_STRUCT
          && rhs_i < pm->str->len && pm->str->str[rhs_i] == ';')
      {
        /* Empty scalar member initializers mean value-initialization. Resolve
           this after the class is complete: an inline constructor can precede
           its fields, so its initial replay does not yet know their types. */
        CValue zero;
        zero.i = 0;
        tok_str_add(dst, this_tok);
        tok_str_add(dst, TOK_ARROW);
        tok_str_add(dst, field_tok);
        tok_str_add(dst, TOK_INIT_MEMBER);
        if (field->type.t & VT_ARRAY) {
          tok_str_add(dst, '{');
          tok_str_add(dst, '}');
        } else
          tok_str_add2(dst, TOK_CINT, &zero);
        tok_str_add(dst, ';');
        i = rhs_i;
        continue;
      }
      if (field && (field->type.t & VT_BTYPE) == VT_STRUCT && field_struct_tok)
      {
        tok_str_add(dst, tok_alloc_const("new"));
        tok_str_add(dst, '(');
        tok_str_add(dst, '&');
        tok_str_add(dst, '(');
        tok_str_add(dst, this_tok);
        tok_str_add(dst, TOK_ARROW);
        tok_str_add(dst, field_tok);
        tok_str_add(dst, ')');
        tok_str_add(dst, ')');
        tok_str_add(dst, TOK_STRUCT);
        tok_str_add(dst, field_struct_tok);
        tok_str_add(dst, '(');
        for (j = i + 4; j < pm->str->len && pm->str->str[j] != ';'; ++j)
        {
          if (TOK_HAS_VALUE(pm->str->str[j]))
          {
            tok_str_add_record(dst, pm->str->str, pm->str->len, &j);
            continue;
          }
          tok_str_add(dst, pm->str->str[j]);
        }
        tok_str_add(dst, ')');
        if (j < pm->str->len)
          tok_str_add(dst, ';');
        i = j;
        continue;
      }
    }
    if (t >= TOK_UIDENT && t != this_tok
        && (i == 0 || (pm->str->str[i - 1] != '.'
                       && pm->str->str[i - 1] != TOK_ARROW
                       && pm->str->str[i - 1] != ':')))
    {
      int nested_type_tok = make_static_member_tok(pm->struct_tok, t);
      if (struct_find(nested_type_tok))
      {
        tok_str_add(dst, nested_type_tok);
        continue;
      }
    }
    if (t >= TOK_UIDENT
        && !strncmp(get_tok_str(t, NULL), "operator", 8)
        && (i == 0 || (pm->str->str[i - 1] != '.'
                       && pm->str->str[i - 1] != TOK_ARROW))
        && i + 1 < pm->str->len && pm->str->str[i + 1] == '(')
    {
      tok_str_add(dst, this_tok);
      tok_str_add(dst, TOK_ARROW);
    }
    if (t == TOK_OPERATOR
        && (i == 0 || (pm->str->str[i - 1] != '.'
                       && pm->str->str[i - 1] != TOK_ARROW))
        && i + 3 < pm->str->len && pm->str->str[i + 1] == '['
        && pm->str->str[i + 2] == ']' && pm->str->str[i + 3] == '(')
    {
      tok_str_add(dst, this_tok);
      tok_str_add(dst, TOK_ARROW);
    }
    if (t >= TOK_IDENT
        && t != this_tok
        && !pending_member_signature_has_param_name(pm->str, t)
        && (i == 0 || (pm->str->str[i - 1] != '.'
                       && pm->str->str[i - 1] != TOK_ARROW
                       && pm->str->str[i - 1] != ':')))
    {
      int dummy_ofs;
      Sym *member = find_field_try(&class_type, t, &dummy_ofs);
      if (member && (member->type.t & VT_BTYPE) != VT_FUNC)
      {
        tok_str_add(dst, this_tok);
        tok_str_add(dst, TOK_ARROW);
      }
    }
    tok_str_add(dst, t);
  }
  tok_str_add(dst, TOK_EOF);
  tok_str_free(pm->str);
  pm->str = dst;
  pm->token_filter_valid = 0;
}

static void rewrite_pending_static_member_calls(PendingMemberFunc *pm)
{
  int i, in_body = 0;
  TokenString *dst;

  if (!pm || !pm->is_static_member || !pm->struct_tok || !pm->str)
    return;
  prepare_saved_cpp_lambdas(pm->str);
  pm->token_filter_valid = 0;
  dst = tok_str_alloc();
  for (i = 0; i < pm->str->len && pm->str->str[i] != TOK_EOF; ++i)
  {
    int t = pm->str->str[i];
    int prev_i, next_i;
    if (TOK_HAS_VALUE(t))
    {
      tok_str_add_record(dst, pm->str->str, pm->str->len, &i);
      continue;
    }
    if (!in_body)
    {
      tok_str_add(dst, t);
      if (t == '{')
        in_body = 1;
      continue;
    }
    if (append_nested_lambda_tokens(dst, pm->str, &i))
      continue;
    prev_i = tok_str_prev_token_index(pm->str->str, pm->str->len, i);
    next_i = tok_str_next_non_linenum_index(pm->str->str, pm->str->len,
                                             i + 1);
    if (t >= TOK_UIDENT
        && (prev_i < 0 || (pm->str->str[prev_i] != '.'
                           && pm->str->str[prev_i] != TOK_ARROW
                           && pm->str->str[prev_i] != ':'
                           && !is_template_keyword_tok(pm->str->str[prev_i])))
        && next_i < pm->str->len
        && (pm->str->str[next_i] == '('
            || pm->str->str[next_i] == TOK_LT
            || pm->str->str[next_i] == '<')
        && !body_identifier_is_decl_target(pm->str, i, prev_i, next_i)
        && class_has_static_member_func(pm->struct_tok, t))
    {
      tok_str_add(dst, pm->struct_tok);
      tok_str_add(dst, ':');
      tok_str_add(dst, ':');
    }
    tok_str_add(dst, t);
  }
  tok_str_add(dst, TOK_EOF);
  tok_str_free(pm->str);
  pm->str = dst;
  pm->token_filter_valid = 0;
}

static void compile_pending_member_funcs(int start)
{
  Sym **saved_local_bindings = NULL;
  int nb_saved_local_bindings = 0;
  int i, saved_tok, out;
  int saved_local_scope;
  int saved_defer_pending_member_funcs;
  int saved_vstack_depth;
  int isolate_local_scope;
  CValue saved_tokc;
  Sym *saved_local_stack;
  SValue *saved_vstack = NULL;

  /* Re-entrancy guard: compiling a replayed member body can itself resolve
     member calls that queue and flush pending members.  A nested flush would
     compact the shared queue while the outer pass is mid-iteration and can
     drop entries the outer pass still owes (e.g. out-of-class template
     operator bodies).  The outer pass iterates with a live bound, so it picks
     up every entry appended during the pass; nested flushes must simply stay
     out of the way. */
  if (compiling_pending_member_funcs || cpp_template_probe_depth)
    return;
  compiling_pending_member_funcs++;

  saved_tok = tok;
  saved_tokc = tokc;
  saved_local_stack = local_stack;
  saved_local_scope = local_scope;
  saved_defer_pending_member_funcs = defer_pending_member_funcs;
  isolate_local_scope = compile_local_member_funcs_now
                        && (saved_local_stack || saved_local_scope);
  saved_vstack_depth = (int)(vtop - vstack + 1);
  if (saved_vstack_depth > 0)
  {
    saved_vstack = cprime_malloc(saved_vstack_depth * sizeof(SValue));
    memcpy(saved_vstack, vstack, saved_vstack_depth * sizeof(SValue));
  }
  vtop = vstack - 1;
  if (isolate_local_scope && saved_local_stack)
  {
    /* Replayed member definitions are emitted as global functions.  Hide
       automatic locals while retaining local type bindings so an inline
       member of a local class can still name its lowered class type. */
    saved_local_bindings = sym_hide_local_bindings(saved_local_stack,
                                                   &nb_saved_local_bindings);
    sym_relink_local_types(saved_local_bindings, nb_saved_local_bindings);
  }
  if (isolate_local_scope)
  {
    local_stack = NULL;
    local_scope = 0;
  }
  out = start;
  for (i = start; i < nb_pending_member_funcs; ++i)
  {
    PendingMemberFunc *pm = pending_member_funcs[i];
    if (!pm)
      continue;
    int guard_non_lifecycle =
      pm->is_template_member
      && !pm->is_lifecycle_member;
    TokenString *member_macro_parent;
    const int *member_macro_ptr;
    int saved_namespace_tok;
    if (compile_lifecycle_member_funcs_only && !pm->is_lifecycle_member)
    {
      pending_member_funcs[out++] = pm;
      continue;
    }
    if (compile_local_member_funcs_now && !pm->is_local_class_member)
    {
      pending_member_funcs[out++] = pm;
      continue;
    }
    if ((local_stack || local_scope) && !compile_local_member_funcs_now)
    {
      pending_member_funcs[out++] = pm;
      continue;
    }
    if (pm->struct_tok)
    {
      Sym *class_sym = struct_find(pm->struct_tok);
      if (!class_sym || class_sym->c < 0)
      {
        pending_member_funcs[out++] = pm;
        continue;
      }
    }
    if (pm->str->len > 0
        && pm->str->str[0] == TOK_AUTO)
    {
      pending_member_funcs[out++] = pm;
      continue;
    }
    /* Instantiating a class does not instantiate an unused member body.
       A selected call or function-address expression demands its return type. */
    if (pm->auto_return_state == 1
        && is_class_template_instantiation_tok(pm->struct_tok)) {
      pending_member_funcs[out++] = pm;
      continue;
    }
    deduce_pending_member_return(pm->func_tok);
    rewrite_pending_member_field_access(pm);
    rewrite_pending_static_member_calls(pm);
    if (getenv("CPC_TRACE_PENDING"))
    {
      const char *filter = getenv("CPC_TRACE_PENDING");
      int pi, matched = 0;
      for (pi = 0; pi < pm->str->len; ++pi)
      {
        if (TOK_HAS_VALUE(pm->str->str[pi]))
        {
          pi += tok_str_value_extra_words(pm->str->str, pm->str->len, pi);
          continue;
        }
        if (strstr(get_tok_str(pm->str->str[pi], NULL), filter))
        {
          matched = 1;
          break;
        }
      }
      if (matched)
      {
        fprintf(stderr, "CPC_PENDING toks=");
        for (pi = 0; pi < pm->str->len && pi < 120; ++pi)
        {
          if (TOK_HAS_VALUE(pm->str->str[pi]))
          {
            fprintf(stderr, "%s<value>", pi ? " " : "");
            pi += tok_str_value_extra_words(pm->str->str, pm->str->len, pi);
            continue;
          }
          fprintf(stderr, "%s%s", pi ? " " : "",
                  get_tok_str(pm->str->str[pi], NULL));
        }
        fprintf(stderr, "\n");
      }
    }
    if (guard_non_lifecycle)
      compiling_non_lifecycle_template_member_body++;
    member_macro_parent = macro_stack;
    member_macro_ptr = macro_ptr;
    saved_namespace_tok = current_namespace_scope_tok();
    enter_namespace_scope(symbol_namespace_scope_tok(pm->struct_tok));
    begin_macro(pm->str, 1);
    next();
    defer_pending_member_funcs = 1;
    if (pm->func_tok && pm->struct_tok)
      note_member_function_scope(pm->func_tok, pm->struct_tok);
    decl(VT_CONST);
    enter_namespace_scope(saved_namespace_tok);
    defer_pending_member_funcs = saved_defer_pending_member_funcs;
    if (pm->func_tok
        && !compiled_pending_member_func_has_tok(pm->func_tok))
    {
      if (nb_compiled_pending_member_func_toks
          >= al_compiled_pending_member_func_toks)
      {
        al_compiled_pending_member_func_toks =
          al_compiled_pending_member_func_toks
            ? al_compiled_pending_member_func_toks * 2 : 32;
        compiled_pending_member_func_toks = cprime_realloc(
          compiled_pending_member_func_toks,
          al_compiled_pending_member_func_toks
            * sizeof(*compiled_pending_member_func_toks));
      }
      compiled_pending_member_func_toks[
        nb_compiled_pending_member_func_toks++] = pm->func_tok;
    }
    /* Each replayed member body is an independent function definition.
       decl() leaves the enclosing local scope live, so restore the saved
       scope before the next queued member is inspected; otherwise a member
       appended mid-flush (e.g. a constructor needed by a static member's
       body) is wrongly deferred forever. */
    if (isolate_local_scope)
    {
      local_stack = NULL;
      local_scope = 0;
    }
    while (macro_stack && macro_stack != member_macro_parent)
      end_macro();
    macro_ptr = member_macro_ptr;
    if (guard_non_lifecycle)
      compiling_non_lifecycle_template_member_body--;
    /* The compiled body's token string is freed by end_macro(); clear the
       queue slot so nested scans (pending_member_func_has_body_tok) cannot
       read the dangling token string before the queue is compacted. */
    pending_member_funcs[i] = NULL;
    /* Member calls discovered while replaying this body stay on the body
       dependency worklist until the outer finalization loop.  Instantiating
       them here would nest template-specialization replay inside the shared
       member queue while it is being compacted. */
  }
  nb_pending_member_funcs = out;
  if (isolate_local_scope && saved_local_stack)
  {
    sym_unlink_local_types(saved_local_bindings, nb_saved_local_bindings);
    sym_relink_stack(saved_local_bindings, nb_saved_local_bindings);
  }
  local_stack = saved_local_stack;
  local_scope = saved_local_scope;
  defer_pending_member_funcs = saved_defer_pending_member_funcs;
  if (saved_vstack_depth > 0)
  {
    memcpy(vstack, saved_vstack, saved_vstack_depth * sizeof(SValue));
    cprime_free(saved_vstack);
  }
  vtop = vstack + saved_vstack_depth - 1;
  tok = saved_tok;
  tokc = saved_tokc;
  compiling_pending_member_funcs--;
}

static void compile_pending_lifecycle_member_funcs(int start)
{
  int saved_compile_lifecycle_member_funcs_only =
    compile_lifecycle_member_funcs_only;
  compile_lifecycle_member_funcs_only = 1;
  compile_pending_member_funcs(start);
  compile_lifecycle_member_funcs_only =
    saved_compile_lifecycle_member_funcs_only;
}

static void parse_namespace_decl(void)
{
  int ns_tok;

  next();
  if (tok == '{')
  {
    if (!unnamed_namespace_tok)
    {
      const unsigned char *p = (const unsigned char *)cprime_state->current_filename;
      unsigned int hash = 2166136261u;
      char name[64];
      /* Template identities containing internal-linkage types must also be
         distinct across translation units. Reopenings in one unit agree. */
      while (p && *p)
        hash = (hash ^ *p++) * 16777619u;
      snprintf(name, sizeof(name), "__cpc_unnamed_%08x", hash);
      unnamed_namespace_tok = tok_alloc_const(name);
    }
    ns_tok = unnamed_namespace_tok;
  }
  else
  {
    if (tok < TOK_UIDENT)
      cprime_error("namespace name");
    ns_tok = tok;
    next();
  }
  note_namespace_tok(ns_tok);
  if (nb_namespace_stack >= (int)(sizeof(namespace_stack) / sizeof(namespace_stack[0])))
    cprime_error("namespace nesting too deep");
  skip('{');
  namespace_stack[nb_namespace_stack++] = ns_tok;
  note_current_namespace_scope();
  while (tok != TOK_EOF && tok != '}')
    decl(VT_CONST);
  skip('}');
  --nb_namespace_stack;
  if (tok == ';')
    next();
}

static int try_parse_using_namespace_directive(void)
{
  int parts[16], nb_parts = 0, namespace_tok, i;
  TokenString *replay;

  if (tok < TOK_UIDENT || strcmp(get_tok_str(tok, NULL), "using"))
    return 0;
  replay = tok_str_alloc();
  tok_str_add2(replay, tok, &tokc);
  next();
  if (tok != TOK_NAMESPACE)
  {
    restore_cpp_lifecycle_probe(replay);
    return 0;
  }
  tok_str_free(replay);
  next();
  if (tok < TOK_UIDENT)
    cprime_error("namespace name expected in using directive");
  for (;;)
  {
    if (nb_parts >= (int)(sizeof(parts) / sizeof(parts[0])))
      cprime_error("namespace name too deep in using directive");
    parts[nb_parts++] = tok;
    next();
    if (tok != ':')
      break;
    next();
    if (tok != ':')
      cprime_error("':' expected in using directive");
    next();
    if (tok < TOK_UIDENT)
      cprime_error("namespace name expected in using directive");
  }
  skip(';');
  namespace_tok = make_namespace_tok_from_parts(parts, nb_parts);
  for (i = 0; i < nb_using_namespace_toks; ++i)
    if (using_namespace_toks[i] == namespace_tok)
      return 1;
  if (nb_using_namespace_toks >= al_using_namespace_toks)
  {
    al_using_namespace_toks = al_using_namespace_toks
                                ? al_using_namespace_toks * 2 : 8;
    using_namespace_toks = cprime_realloc(
      using_namespace_toks, al_using_namespace_toks * sizeof(int));
  }
  using_namespace_toks[nb_using_namespace_toks++] = namespace_tok;
  return 1;
}
