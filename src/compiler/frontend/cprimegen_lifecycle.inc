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
    s = sym_find2(global_stack, mangled_tok);
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
  Sym *s = resolve_lifecycle_func(type, TOK_DESTRUCTOR1);
  if (!lifecycle_has_only_this_param(s))
    return NULL;
  return s;
}

static Sym *resolve_autoctor_func(CType *type)
{
  Sym *s = resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1, 0);
  if (!s)
    s = resolve_lifecycle_func(type, TOK_CONSTRUCTOR1);
  if (s && !lifecycle_has_only_this_param(s)
      && member_func_min_arg_count(&s->type) > 0)
    return NULL;
  return s;
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
  if (struct_tok && name_tok != struct_tok)
  {
    char expected[512];
    snprintf(expected, sizeof(expected), "%s__",
             get_tok_str(name_tok, NULL));
    if (strncmp(get_tok_str(struct_tok, NULL), expected, strlen(expected)))
      cprime_error("destructor name must match class name");
  }

  func_sym = resolve_lifecycle_func(type, TOK_DESTRUCTOR1);
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

  mk_pointer(&vtop->type);
  gaddrof();
  vpushsym(&func_sym->type, func_sym);
  vswap();
  finish_cpp_member_func_call(func_sym->type.ref, 1);
}

static void call_lifecycle_constructor_raw(CType *type, int r, int addr, Sym *sym,
                                           Sym *ctor_func)
{
  Sym *func_type, *sa;
  TokenString *call_args[32];
  CType call_arg_types[32];
  int nb_args, call_arg_count, ai;

  skip('(');
  skip_line_markers();
  if (tok == '{')
    call_arg_count = count_saved_braced_ctor_args(call_args, 32);
  else
    call_arg_count = count_saved_call_args(call_args, 32);
  infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
  {
    Sym *overload = resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                     call_arg_types,
                                                     call_arg_count);
    if (!overload)
      overload = resolve_member_field_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                        call_arg_types,
                                                        call_arg_count);
    if (!overload && call_arg_count == 1
        && is_same_template_family_conversion_ctor(type, &call_arg_types[0]))
    {
      next();
      return;
    }
    if (!overload)
      overload = resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1,
                                                  call_arg_count);
    if (overload)
      ctor_func = overload;
  }
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

  for (ai = 0; ai < call_arg_count; ++ai)
  {
    if (saved_arg_is_braced(call_args[ai]) && sa
        && (((sa->type.t & VT_BTYPE) == VT_STRUCT)
            || (is_reference_type(&sa->type)
                && ((pointed_type(&sa->type)->t & VT_BTYPE) == VT_STRUCT))))
    {
      CType temp_type;
      int arg_size, arg_align, arg_r2, arg_addr;
      temp_type = is_reference_type(&sa->type) ? *pointed_type(&sa->type)
                                               : sa->type;
      arg_size = type_size(&temp_type, &arg_align);
      arg_addr = get_temp_local_var(arg_size, arg_align, &arg_r2);
      vset(&temp_type, VT_LOCAL | VT_LVAL, arg_addr);
      vtop->r2 = arg_r2;
    }
    else
    {
      begin_macro(call_args[ai], 1);
      next();
      expr_eq();
      end_macro();
    }
    gfunc_param_typed(func_type, sa);
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
      ctor_func = resolve_lifecycle_func(elem_type, TOK_CONSTRUCTOR1);
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
    ctor_func = resolve_lifecycle_func(&field->type, TOK_CONSTRUCTOR1);
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

  if (!ctor_func)
    return;
  if ((ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref)
    cprime_error("constructor target is not declared as function");

  mk_pointer(&vtop->type);
  gaddrof();
  vpushsym(&ctor_func->type, ctor_func);
  vswap();

  func_type = ctor_func->type.ref;
  sa = func_type->next;
  gfunc_param_typed(func_type, sa);
  if (sa)
    sa = sa->next;
  if (sa)
    cprime_error("too few arguments to constructor");
  gfunc_call(1);
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
      ctor_func = resolve_lifecycle_func(elem_type, TOK_CONSTRUCTOR1);
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
    ctor_func = resolve_lifecycle_func(&field->type, TOK_CONSTRUCTOR1);
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

  if (resolve_copy_constructor_func(type, type))
    return 1;

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
                                           SValue *src_ptr, int offset)
{
  SValue *saved_vtop = vtop;
  Sym *ctor_func, *ctor_type, *sa;

  ctor_func = resolve_copy_constructor_func(type, type);
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
      if ((elem_type->t & VT_BTYPE) == VT_STRUCT
          && !resolve_copy_constructor_func(elem_type, elem_type))
        copy_construct_struct_memberwise_from_base_ptr(elem_type, dst_ptr,
                                                       src_ptr, elem_offset);
      else
        copy_construct_one_lvalue_pair(elem_type, dst_ptr, src_ptr,
                                       elem_offset);
    }
    return;
  }

  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
  {
    copy_construct_one_lvalue_pair(type, dst_ptr, src_ptr, base_offset);
    return;
  }

  for (field = type->ref->next; field; field = field->next)
  {
    int field_offset = base_offset + field->c;
    if ((field->type.t & VT_BTYPE) == VT_STRUCT
        && !resolve_copy_constructor_func(&field->type, &field->type))
      copy_construct_struct_memberwise_from_base_ptr(&field->type, dst_ptr,
                                                     src_ptr, field_offset);
    else
      copy_construct_one_lvalue_pair(&field->type, dst_ptr, src_ptr,
                                     field_offset);
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

        if ((elem_type->t & VT_BTYPE) == VT_STRUCT)
          register_struct_cleanups(elem_type, sym);

        cleanup_func = resolve_autodtor_func(elem_type);
        if (!cleanup_func)
          continue;

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

    if ((field->type.t & VT_BTYPE) == VT_STRUCT)
      register_struct_cleanups(&field->type, sym);

    cleanup_func = resolve_autodtor_func(&field->type);
    if (!cleanup_func)
      continue;

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
  call_lifecycle_constructor_members(type, r, addr);
  call_lifecycle_constructor_raw(type, r, addr, sym, ctor_func);
}

static void call_lifecycle_constructor_saved_args(CType *type, int r, int addr,
                                                  Sym *sym, Sym *ctor_func,
                                                  TokenString **call_args,
                                                  int call_arg_count)
{
  Sym *func_type, *sa;
  CType call_arg_types[32];
  int nb_args, ai, saved_delim;

  saved_delim = tok;
  infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
  if (tok == TOK_EOF)
  {
    next();
    unget_tok(saved_delim);
  }
  {
    Sym *overload = resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                     call_arg_types,
                                                     call_arg_count);
    int same_type_copy_init = 0;
    if (call_arg_count == 1)
    {
      CType arg_value_type = call_arg_types[0];
      arg_value_type.t &= ~VT_RVALUE_REFERENCE;
      same_type_copy_init = is_compatible_unqualified_types(type,
                                                            &arg_value_type);
    }
    if (same_type_copy_init)
    {
      init_params p = {0};
      TokenString macro = *call_args[0];
      begin_macro(&macro, 0);
      next();
      expr_eq();
      end_macro();
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
    if (!overload)
      overload = resolve_member_field_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                        call_arg_types,
                                                        call_arg_count);
    if (overload)
      ctor_func = overload;
    else if (call_arg_count == 1
             && is_same_template_family_conversion_ctor(type, &call_arg_types[0]))
    {
      call_lifecycle_constructor_members(type, r, addr);
      return;
    }
    else
    {
      overload = resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1,
                                                  call_arg_count);
      if (overload)
        ctor_func = overload;
      else
        cprime_error("no matching constructor for copy-initialization");
    }
  }
  if (!ctor_func || (ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref)
    cprime_error("constructor target is not declared as function");


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
    if (saved_arg_is_braced(call_args[ai]) && sa
        && (((sa->type.t & VT_BTYPE) == VT_STRUCT)
            || (is_reference_type(&sa->type)
                && ((pointed_type(&sa->type)->t & VT_BTYPE) == VT_STRUCT))))
    {
      CType temp_type;
      AttributeDef temp_ad;
      TokenString macro;
      temp_type = is_reference_type(&sa->type) ? *pointed_type(&sa->type)
                                               : sa->type;
      memset(&temp_ad, 0, sizeof temp_ad);
      macro = *call_args[ai];
      begin_macro(&macro, 0);
      next();
      decl_initializer_alloc(&temp_type, &temp_ad, VT_LOCAL | VT_LVAL,
                             1, 0, NULL, 0, VT_LOCAL);
      end_macro();
    }
    else
    {
      TokenString macro = *call_args[ai];
      begin_macro(&macro, 0);
      next();
      expr_eq();
      end_macro();
    }
    gfunc_param_typed(func_type, sa);
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

static int try_parse_cpp_functional_constructor(int type_tok)
{
  Sym *struct_sym, *ctor_func;
  TokenString *call_args[32];
  CType call_arg_types[32];
  CType type;
  int size, align, addr, r2, call_arg_count;

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
      alias = sym_find2(global_stack, type_tok);
    if (!alias || !(alias->type.t & VT_TYPEDEF)
        || (alias->type.t & VT_BTYPE) != VT_STRUCT)
      return 0;
    type = alias->type;
    type.t &= ~VT_TYPEDEF;
  }

  if (tok == '{')
  {
    AttributeDef ad;

    memset(&ad, 0, sizeof ad);
    decl_initializer_alloc(&type, &ad, VT_LOCAL | VT_LVAL, 1, 0, NULL, 0,
                           VT_LOCAL);
    return 1;
  }

  if (tok != '(')
    return 0;

  next();
  skip_line_markers();
  if (tok == '{')
    call_arg_count = count_saved_braced_ctor_args(call_args, 32);
  else
    call_arg_count = count_saved_call_args(call_args, 32);
  infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
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
      cprime_error("no matching constructor for functional-style construction");
    size = type_size(&type, &align);
    addr = get_temp_local_var(size, align, &r2);
    call_lifecycle_constructor_members(&type, VT_LOCAL | VT_LVAL, addr);
    if (tok == TOK_EOF)
      next();
    else
      skip(')');
    vset(&type, VT_LOCAL | VT_LVAL, addr);
    vtop->r2 = r2;
    return 1;
  }

  size = type_size(&type, &align);
  addr = get_temp_local_var(size, align, &r2);
  call_lifecycle_constructor_saved_args(&type, VT_LOCAL | VT_LVAL, addr,
                                        NULL, ctor_func, call_args,
                                        call_arg_count);
  if (tok == TOK_EOF)
    next();
  else
    skip(')');

  vset(&type, VT_LOCAL | VT_LVAL, addr);
  vtop->r2 = r2;
  return 1;
}

static void call_placement_constructor(CType *type, TokenString *placement,
                                       TokenString **call_args,
                                       int call_arg_count)
{
  Sym *ctor_func, *func_type, *sa;
  CType call_arg_types[32], ptr_type;
  int nb_args, ai, saved_tok;
  CValue saved_tokc;

  infer_saved_arg_types(call_args, call_arg_types, call_arg_count);
  ctor_func = resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                               call_arg_types,
                                               call_arg_count);
  if (!ctor_func)
    ctor_func = resolve_member_field_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                       call_arg_types,
                                                       call_arg_count);
  if (!ctor_func)
    ctor_func = resolve_member_func_by_arg_count(type, TOK_CONSTRUCTOR1,
                                                call_arg_count);
  if (!ctor_func)
    ctor_func = resolve_lifecycle_func(type, TOK_CONSTRUCTOR1);
  if (ctor_func
      && ((ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref))
    cprime_error("placement constructor target is not a function");

  saved_tok = tok;
  saved_tokc = tokc;
  begin_macro(placement, 1);
  next();
  expr_eq();
  end_macro();
  tok = saved_tok;
  tokc = saved_tokc;

  ptr_type = *type;
  mk_pointer(&ptr_type);
  gen_cast(&ptr_type);

  if (!ctor_func)
  {
    SValue base_ptr = *vtop;
    call_lifecycle_constructor_members_base_ptr(type, &base_ptr, 0);
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
    saved_tok = tok;
    saved_tokc = tokc;
    begin_macro(call_args[ai], 1);
    next();
    expr_eq();
    end_macro();
    tok = saved_tok;
    tokc = saved_tokc;
    gfunc_param_typed(func_type, sa);
    nb_args++;
    if (sa)
      sa = sa->next;
  }
  if (sa)
    cprime_error("too few arguments to constructor");

  vcheck_cmp();
  gfunc_call(nb_args);
  drop_leaked_call_target(ctor_func);
  if ((vtop->type.t & VT_BTYPE) == VT_VOID)
    vpop();
}

static int try_parse_cpp_placement_new_after_name(void)
{
  CType type;
  AttributeDef ad;
  TokenString *placement, *call_args[32];
  int call_arg_count = 0, saved_tok;
  CValue saved_tokc;

  if (tok != '(')
    return 0;

  next();
  skip_or_save_block(&placement);
  skip(')');

  memset(&ad, 0, sizeof(ad));
  if (!parse_btype(&type, &ad, 0))
    cprime_error("placement new requires a type");

  if (tok == '(')
  {
    next();
    call_arg_count = count_saved_call_args(call_args, 32);
    skip(')');
  }
  if ((type.t & VT_BTYPE) != VT_STRUCT)
  {
    CType ptr_type = type;
    int i;

    if (call_arg_count > 1)
      cprime_error("scalar placement new accepts at most one argument");
    saved_tok = tok;
    saved_tokc = tokc;
    begin_macro(placement, 1);
    next();
    expr_eq();
    end_macro();
    tok = saved_tok;
    tokc = saved_tokc;
    mk_pointer(&ptr_type);
    gen_cast(&ptr_type);
    vdup();
    indir();
    if (call_arg_count)
    {
      begin_macro(call_args[0], 1);
      next();
      expr_eq();
      end_macro();
      tok = saved_tok;
      tokc = saved_tokc;
    }
    else
      vpushi(0);
    vstore();
    vpop();
    for (i = 0; i < call_arg_count; ++i)
      tok_str_free(call_args[i]);
    tok_str_free(placement);
    return 1;
  }
  else if (!type.ref->a.lifecycle_ctor)
  {
    int saved_tok = tok;
    CValue saved_tokc = tokc;
    begin_macro(placement, 1);
    next();
    expr_eq();
    end_macro();
    tok = saved_tok;
    tokc = saved_tokc;
    mk_pointer(&type);
    gen_cast(&type);
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
  owner_tok = struct_tok;
  if (!is_lifecycle_member_tok(method_tok))
    instantiate_template_member_for_call(type, method_tok, NULL, -1);
  mangled_tok = make_member_func_tok(struct_tok, method_tok);
  s = sym_find(mangled_tok);
  if (!s)
    s = sym_find2(global_stack, mangled_tok);
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

static int member_overload_receiver_ok(MemberFuncOverload *o, int receiver_const)
{
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
  if (receiver_const)
    return const_match;
  return match ? match : const_match;
}

static Sym *resolve_member_func_by_arg_count(CType *type, int method_tok,
                                             int explicit_arg_count)
{
  int struct_tok;
  MemberFuncOverload *o;
  Sym *match = NULL;
  Sym *const_match = NULL;
  int receiver_const = member_receiver_is_const(type);
  int best_omitted_defaults = 0x7fffffff;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  instantiate_template_member_for_call(type, method_tok, NULL,
                                       explicit_arg_count);
  for (o = member_func_overloads; o;)
  {
    MemberFuncOverload *next_o = o->next;
    Sym *s;
    if (o->struct_tok != struct_tok || o->method_tok != method_tok
        || explicit_arg_count < o->min_arg_count
        || explicit_arg_count > o->explicit_arg_count)
      goto next_overload;
    if (!member_overload_receiver_ok(o, receiver_const))
      goto next_overload;
    s = sym_find2(global_stack, o->mangled_tok);
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
  if (match)
    return match;
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

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return 0;
  for (o = member_func_overloads; o; o = o->next)
    if (o->struct_tok == struct_tok && o->method_tok == method_tok
        && explicit_arg_count >= o->min_arg_count
        && explicit_arg_count <= o->explicit_arg_count)
      return 1;
  return 0;
}

static int member_overload_count_for_call(CType *type, int method_tok,
                                          int explicit_arg_count)
{
  int struct_tok, count = 0;
  MemberFuncOverload *o;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return 0;
  for (o = member_func_overloads; o; o = o->next)
    if (o->struct_tok == struct_tok && o->method_tok == method_tok
        && explicit_arg_count >= o->min_arg_count
        && explicit_arg_count <= o->explicit_arg_count)
      ++count;
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

  for (o = member_func_overloads; o; o = o->next)
  {
    Sym *s;
    if (o->struct_tok != class_tok || o->method_tok != TOK_CONSTRUCTOR1
        || o->min_arg_count > 1 || o->explicit_arg_count < 1)
      continue;
    s = sym_find2(global_stack, o->mangled_tok);
    if (!s)
      s = sym_find(o->mangled_tok);
    if (!s)
      continue;
    use_overload_func_type(s, &o->func_type);
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
  SValue source;
  Sym *ctor_func, *ctor_type, *sa;
  int size, align, addr, r2;

  if ((type->t & VT_BTYPE) != VT_STRUCT)
    return 0;
  ctor_func = resolve_member_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                               &vtop->type, 1);
  if (!ctor_func)
    ctor_func = resolve_member_field_func_by_arg_types(type, TOK_CONSTRUCTOR1,
                                                       &vtop->type, 1);
  if (!ctor_func)
    return 0;
  if ((ctor_func->type.t & VT_BTYPE) != VT_FUNC || !ctor_func->type.ref)
    cprime_error("constructor target is not declared as function");

  source = *vtop;
  vtop--;

  size = type_size(type, &align);
  addr = get_temp_local_var(size, align, &r2);
  call_lifecycle_constructor_members(type, VT_LOCAL | VT_LVAL, addr);
  vset(type, VT_LOCAL | VT_LVAL, addr);
  vtop->r2 = r2;
  mk_pointer(&vtop->type);
  gaddrof();

  vpushsym(&ctor_func->type, ctor_func);
  vswap();
  ctor_type = ctor_func->type.ref;
  sa = ctor_type->next;
  gfunc_param_typed(ctor_type, sa);
  if (sa)
    sa = sa->next;

  vpushv(&source);
  gfunc_param_typed(ctor_type, sa);
  if (sa)
    sa = sa->next;
  if (sa)
    cprime_error("too few arguments to constructor");

  vcheck_cmp();
  gfunc_call(2);
  drop_leaked_call_target(ctor_func);

  vset(type, VT_LOCAL | VT_LVAL, addr);
  vtop->r2 = r2;
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
      if (td->inst_name_toks[j] == class_tok)
        class_seen = 1;
      if (td->inst_name_toks[j] == arg_tok)
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

static Sym *resolve_member_field_func_by_arg_types(CType *type, int method_tok,
                                                   CType *arg_types,
                                                   int explicit_arg_count)
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
    rank = member_field_func_arg_match_rank(field, arg_types,
                                            explicit_arg_count);
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

static Sym *resolve_member_func_by_arg_types(CType *type, int method_tok,
                                             CType *arg_types,
                                             int explicit_arg_count)
{
  int struct_tok;
  MemberFuncOverload *o;
  Sym *match = NULL;
  Sym *const_match = NULL;
  int receiver_const = member_receiver_is_const(type);
  int best_omitted_defaults = 0x7fffffff;
  int best_arg_rank = 0x7fffffff;

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  instantiate_template_member_for_call(type, method_tok, arg_types,
                                       explicit_arg_count);
  if (method_tok == TOK_CONSTRUCTOR1 && nb_pending_member_funcs)
    compile_pending_member_funcs(0);
  for (o = member_func_overloads; o;)
  {
    MemberFuncOverload *next_o = o->next;
    Sym *s;
    if (o->struct_tok != struct_tok || o->method_tok != method_tok
        || explicit_arg_count < o->min_arg_count
        || explicit_arg_count > o->explicit_arg_count)
      goto next_overload;
    if (!member_overload_receiver_ok(o, receiver_const))
      goto next_overload;
    s = sym_find2(global_stack, o->mangled_tok);
    if (!s)
      s = sym_find(o->mangled_tok);
    if (s)
      use_overload_func_type(s, &o->func_type);
    if (method_tok == TOK_CONSTRUCTOR1 && explicit_arg_count == 1
        && !is_compatible_unqualified_types(type, &arg_types[0])
        && lowered_member_func_single_param_is_self(type, s))
      goto next_overload;
    {
      int arg_rank = member_func_arg_match_rank(s, arg_types,
                                                explicit_arg_count);
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
      if (*slot && *slot != s
          && same_lowered_member_func_signature(&(*slot)->type, &s->type))
        goto next_overload;
    }
    member_overload_note_ranked_match(&match, &const_match,
                                      &best_omitted_defaults, s,
                                      o->is_const,
                                      o->explicit_arg_count - explicit_arg_count);
next_overload:
    o = next_o;
  }
  match = member_overload_pick_match(match, const_match, receiver_const);
  if (match)
    return match;
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
    if (!is_compatible_types(&member_arg->type, &def_arg->type))
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
  int receiver_const = member_receiver_is_const(type);

  struct_tok = get_struct_type_name_tok(type);
  if (!struct_tok)
    return NULL;
  for (o = member_func_overloads; o; o = o->next)
  {
    Sym *s;
    if (o->struct_tok != struct_tok || o->method_tok != method_tok)
      continue;
    if (!member_overload_receiver_ok(o, receiver_const))
      continue;
    s = sym_find(o->mangled_tok);
    if (!s)
      s = sym_find2(global_stack, o->mangled_tok);
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

static CType make_lowered_member_func_type(CType *struct_type, CType *func_type)
{
  CType lowered_type, return_type, this_type, arg_type;
  Sym *fref, *param, *arg, *last;
  Sym *saved_ls;

  return_type = func_type->ref->type;
  lowered_type.t = VT_FUNC;
  /* Same as make_lifecycle_func_type: keep these type symbols off a live
     local scope so scope pops cannot unlink the 'this' parameter symbols. */
  saved_ls = local_stack;
  local_stack = NULL;
  lowered_type.ref = fref = sym_push(SYM_FIELD, &return_type, 0, 0);
  fref->f = func_type->ref->f;

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

static void add_pending_member_func(CType *struct_type, int method_tok,
                                    CType *func_type, TokenString *body)
{
  int struct_tok, mangled_tok, i;
  Sym *arg;
  PendingMemberFunc *pm;
  TokenString *str;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("inline member functions require a named struct/class");
  mangled_tok = make_member_func_tok_for_type(struct_tok, method_tok, func_type);
  {
    CType lowered_type = make_lowered_member_func_type(struct_type, func_type);
    external_global_sym(mangled_tok, &lowered_type);
    note_member_func_overload(struct_tok, method_tok, mangled_tok, &lowered_type);
  }

  str = tok_str_alloc();
  tok_str_add(str, TOK_STATIC);
  tok_str_add(str, TOK_INLINE1);
  if (!add_ctype_tokens(str, &func_type->ref->type))
    cprime_error("unsupported member function return type");
  tok_str_add(str, mangled_tok);
  tok_str_add(str, '(');
  tok_str_add(str, TOK_STRUCT);
  tok_str_add(str, struct_tok);
  if (func_type->t & VT_CONSTANT)
    tok_str_add(str, TOK_CONST1);
  tok_str_add(str, '*');
  tok_str_add(str, tok_alloc_const("this"));
  for (arg = func_type->ref->next; arg; arg = arg->next)
  {
    tok_str_add(str, ',');
    if (!add_ctype_tokens(str, &arg->type))
      cprime_error("unsupported member function parameter type");
    tok_str_add(str, arg->v & ~SYM_FIELD);
  }
  tok_str_add(str, ')');
  for (i = 0; i < body->len && body->str[i] != TOK_EOF; ++i)
  {
    int prev_body_tok = 0, pi;
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
        && (i == 0 || body->str[i - 1] != ':')
        && (i + 1 >= body->len || body->str[i + 1] != ':')
        && class_tok_matches_unqualified_name(struct_tok, body->str[i]))
    {
      tok_str_add(str, struct_tok);
      continue;
    }
    if (body->str[i] >= TOK_UIDENT && i + 1 < body->len
        && body->str[i + 1] == '('
        && prev_body_tok != '.'
        && prev_body_tok != TOK_ARROW
        && prev_body_tok != ':'
        && body->str[i] != struct_tok
        && (class_has_static_member_func(struct_tok, body->str[i])
            || type_has_member_func_name(struct_type, body->str[i])))
    {
      if (class_has_static_member_func(struct_tok, body->str[i]))
      {
        /* Static member calls cannot go through this->; qualify them as
           Class::Member. */
        tok_str_add(str, struct_tok);
        tok_str_add(str, ':');
        tok_str_add(str, ':');
      }
      else
      {
        tok_str_add(str, tok_alloc_const("this"));
        tok_str_add(str, TOK_ARROW);
      }
    }
    tok_str_add(str, body->str[i]);
  }
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  pm->struct_tok = struct_tok;
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
  mangled_tok = make_free_func_tok_for_type(base_tok, &static_type);
  note_free_func_overload(base_tok, mangled_tok, &static_type);
  if (!sym_find(base_tok) && !sym_find2(global_stack, base_tok))
    external_global_sym(base_tok, &static_type);
  return external_global_sym(mangled_tok, &static_type);
}

static void add_pending_static_member_func(CType *struct_type, int method_tok,
                                            CType *func_type, TokenString *body,
                                            int is_auto_return)
{
  int struct_tok, base_tok, mangled_tok, i;
  Sym *arg;
  CType static_type;
  PendingMemberFunc *pm;
  TokenString *str;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("inline static member functions require a named struct/class");

  if (is_auto_return && func_type && func_type->ref && body)
    for (i = 0; i + 1 < body->len; ++i)
      if (body->str[i] == TOK_RETURN)
      {
        int ri = i + 1;
        Sym *return_func;
        while (ri < body->len && body->str[ri] == TOK_LINENUM)
          ri += 2;
        return_func = ri < body->len ? sym_find(body->str[ri]) : NULL;
        if (return_func && (return_func->type.t & VT_BTYPE) == VT_FUNC
            && return_func->type.ref)
          func_type->ref->type = return_func->type.ref->type;
        break;
      }

  declare_static_member_func(struct_type, method_tok, func_type);

  base_tok = make_static_member_tok(struct_tok, method_tok);
  static_type = *func_type;
  static_type.t &= ~VT_STATIC;
  mangled_tok = make_free_func_tok_for_type(base_tok, &static_type);

  str = tok_str_alloc();
  tok_str_add(str, TOK_STATIC);
  tok_str_add(str, TOK_INLINE1);
  if (!add_ctype_tokens(str, &func_type->ref->type))
    cprime_error("unsupported static member function return type");
  tok_str_add(str, mangled_tok);
  tok_str_add(str, '(');
  for (arg = func_type->ref->next; arg; arg = arg->next)
  {
    if (arg != func_type->ref->next)
      tok_str_add(str, ',');
    if (!add_ctype_tokens(str, &arg->type))
      cprime_error("unsupported static member function parameter type");
    tok_str_add(str, arg->v & ~SYM_FIELD);
  }
  tok_str_add(str, ')');
  for (i = 0; i < body->len && body->str[i] != TOK_EOF; ++i)
  {
    int bt = body->str[i];
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
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static void add_lifecycle_param_tokens(TokenString *str, TokenString *params,
                                       int struct_tok)
{
  int i;
  const char *struct_name = get_tok_str(struct_tok, NULL);

  if (!params)
    return;
  for (i = 0; i < params->len && params->str[i] != TOK_EOF; ++i)
  {
    int lt = i + 1;
    while (lt + 1 < params->len && params->str[lt] == TOK_LINENUM)
      lt += 2;
    if (params->str[i] >= TOK_IDENT && lt < params->len
        && params->str[lt] == '<')
    {
      const char *param_name = get_tok_str(params->str[i], NULL);
      int is_self = !strcmp(param_name, struct_name)
                    || (!strncmp(param_name, struct_name, strlen(struct_name))
                        && param_name[strlen(struct_name)] == '_'
                        && param_name[strlen(struct_name) + 1] == '_')
                    || (!strncmp(struct_name, param_name, strlen(param_name))
                        && struct_name[strlen(param_name)] == '_'
                        && struct_name[strlen(param_name) + 1] == '_');
      if (is_self)
      {
        int level = 1;
        tok_str_add(str, params->str[i]);
        i = lt + 1;
        while (i < params->len && params->str[i] != TOK_EOF && level > 0)
        {
          if (params->str[i] == '<')
            level++;
          else if (params->str[i] == TOK_GT || params->str[i] == TOK_SAR)
            level--;
          i++;
        }
        i--;
        continue;
      }
    }
    tok_str_add(str, params->str[i]);
  }
}

static void add_pending_lifecycle_func(CType *struct_type, int method_tok,
                                       TokenString *params,
                                       TokenString *body, int internal)
{
  int struct_tok, mangled_tok, i;
  int empty_body;
  CType ret_type, func_type, lowered_type;
  PendingMemberFunc *pm;
  TokenString *str;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("inline constructors/destructors require a named struct/class");
  ret_type.t = VT_VOID;
  ret_type.ref = NULL;
  func_type = make_func_type_from_saved_params(&ret_type, params);
  lowered_type = make_lowered_member_func_type(struct_type, &func_type);
  mangled_tok = make_lifecycle_func_tok_for_type(struct_tok, method_tok,
                                                &lowered_type, 1);
  if (pending_member_func_has_body_tok(mangled_tok))
    return;
  note_raw_lifecycle_overload(struct_tok, method_tok, mangled_tok, params);
  external_global_sym(mangled_tok, &lowered_type);
  note_member_func_overload(struct_tok, method_tok, mangled_tok,
                            &lowered_type);
  empty_body = token_string_contains_tok(body, tok_alloc_const("begin"));

  str = tok_str_alloc();
  if (internal)
  {
    tok_str_add(str, TOK_STATIC);
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
  if (empty_body)
  {
    tok_str_add(str, '{');
    tok_str_add(str, '}');
  }
  else
  {
    for (i = 0; i < body->len && body->str[i] != TOK_EOF; ++i)
      tok_str_add(str, body->str[i]);
  }
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  pm->struct_tok = struct_tok;
  pm->is_lifecycle_member = 1;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static void add_pending_lifecycle_decl(CType *struct_type, int method_tok,
                                       TokenString *params)
{
  int struct_tok, mangled_tok;
  CType ret_type, func_type, lowered_type;
  PendingMemberFunc *pm;
  TokenString *str;

  struct_tok = get_struct_type_name_tok(struct_type);
  if (!struct_tok)
    cprime_error("constructors/destructors require a named struct/class");
  ret_type.t = VT_VOID;
  ret_type.ref = NULL;
  func_type = make_func_type_from_saved_params(&ret_type, params);
  lowered_type = make_lowered_member_func_type(struct_type, &func_type);
  mangled_tok = make_lifecycle_func_tok_for_type(struct_tok, method_tok,
                                                &lowered_type, 0);
  if (pending_member_func_has_tok(mangled_tok))
    return;
  if (sym_find(mangled_tok) || sym_find2(global_stack, mangled_tok))
    return;
  note_raw_lifecycle_overload(struct_tok, method_tok, mangled_tok, params);
  external_global_sym(mangled_tok, &lowered_type);
  note_member_func_overload(struct_tok, method_tok, mangled_tok,
                            &lowered_type);

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
  tok_str_add(str, ';');
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  pm->struct_tok = struct_tok;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static int can_lower_global_dynamic_init(CType *type, int l, int has_init)
{
  int bt;

  if (l != VT_CONST || !has_init)
    return 0;
  if (type->t & (VT_CONSTANT | VT_ARRAY))
    return 0;
  bt = type->t & VT_BTYPE;
  if (bt == VT_STRUCT)
    return get_struct_type_name_tok(type) && type->ref
           && type->ref->a.lifecycle_ctor;
  return bt != VT_FUNC && bt != VT_VOID;
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
            instantiate_template_member_if_needed(
                md, td->inst_type_toks[j * td->nb_type_params],
                struct_tok, 0);
        }
  for (o = member_func_overloads; o; o = o->next)
  {
    Sym *param, *s;
    CType *param_type;

    if (o->struct_tok != struct_tok || o->method_tok != TOK_CONSTRUCTOR1
        || o->explicit_arg_count != 1)
      continue;
    s = sym_find2(global_stack, o->mangled_tok);
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
    return s;
  }
  return NULL;
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
    tok_str_add(str, struct_tok);
    tok_str_add(str, '(');
  }
  else
  {
    tok_str_add(str, var_tok);
    tok_str_add(str, '=');
  }
  for (i = 0; i < init->len && init->str[i] != TOK_EOF; ++i)
    tok_str_add(str, init->str[i]);
  if ((type->t & VT_BTYPE) == VT_STRUCT)
    tok_str_add(str, ')');
  tok_str_add(str, ';');
  tok_str_add(str, '}');
  tok_str_add(str, TOK_EOF);

  pm = cprime_mallocz(sizeof(*pm));
  pm->str = str;
  dynarray_add(&pending_member_funcs, &nb_pending_member_funcs, pm);
}

static void rewrite_pending_member_field_access(PendingMemberFunc *pm)
{
  int i, in_body = 0, this_tok = tok_alloc_const("this");
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
  dst = tok_str_alloc();
  for (i = 0; i < pm->str->len && pm->str->str[i] != TOK_EOF; ++i)
  {
    int t = pm->str->str[i];
    if (!in_body)
    {
      tok_str_add(dst, t);
      if (t == '{')
        in_body = 1;
      continue;
    }
    if (t == this_tok && i + 3 < pm->str->len
        && pm->str->str[i + 1] == TOK_ARROW
        && pm->str->str[i + 2] >= TOK_UIDENT
        && pm->str->str[i + 3] == '=')
    {
      int field_tok = pm->str->str[i + 2], dummy_ofs, field_struct_tok, j;
      int rhs_i = i + 4;
      Sym *field = find_field_try(&class_type, field_tok, &dummy_ofs);
      field_struct_tok = field ? get_struct_type_name_tok(&field->type) : 0;
      while (rhs_i + 1 < pm->str->len
             && pm->str->str[rhs_i] == TOK_LINENUM)
        rhs_i += 2;
      if (rhs_i + 1 < pm->str->len
          && pm->str->str[rhs_i] >= TOK_UIDENT
          && pm->str->str[rhs_i + 1] == TOK_ARROW)
        goto keep_field_assignment;
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
        tok_str_add(dst, field_struct_tok);
        tok_str_add(dst, '(');
        for (j = i + 4; j < pm->str->len && pm->str->str[j] != ';'; ++j)
          tok_str_add(dst, pm->str->str[j]);
        tok_str_add(dst, ')');
        if (j < pm->str->len)
          tok_str_add(dst, ';');
        i = j;
        continue;
      }
    }
keep_field_assignment:
    if (t >= TOK_IDENT
        && t != this_tok
        && (i == 0 || (pm->str->str[i - 1] != '.'
                       && pm->str->str[i - 1] != TOK_ARROW
                       && pm->str->str[i - 1] != ':'))
        && (i + 1 >= pm->str->len || pm->str->str[i + 1] != '('))
    {
      int dummy_ofs;
      if (find_field_try(&class_type, t, &dummy_ofs))
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
}

static void compile_pending_member_funcs(int start)
{
  int i, saved_tok, out, len_tok, array_tok;
  int saved_local_scope;
  int saved_defer_pending_member_funcs;
  CValue saved_tokc;
  Sym *saved_local_stack;

  /* Re-entrancy guard: compiling a replayed member body can itself resolve
     member calls that queue and flush pending members.  A nested flush would
     compact the shared queue while the outer pass is mid-iteration and can
     drop entries the outer pass still owes (e.g. out-of-class template
     operator bodies).  The outer pass iterates with a live bound, so it picks
     up every entry appended during the pass; nested flushes must simply stay
     out of the way. */
  if (compiling_pending_member_funcs)
    return;
  compiling_pending_member_funcs++;

  saved_tok = tok;
  saved_tokc = tokc;
  saved_local_stack = local_stack;
  saved_local_scope = local_scope;
  saved_defer_pending_member_funcs = defer_pending_member_funcs;
  len_tok = tok_alloc_const("_len");
  array_tok = tok_alloc_const("_array");
  out = start;
  for (i = start; i < nb_pending_member_funcs; ++i)
  {
    PendingMemberFunc *pm = pending_member_funcs[i];
    if (!pm)
      continue;
    int guard_non_lifecycle =
      pm->is_template_member
      && !pm->is_lifecycle_member;
    int compile_in_global_scope = 0;
    if (compile_lifecycle_member_funcs_only && !pm->is_lifecycle_member)
    {
      pending_member_funcs[out++] = pm;
      continue;
    }
    if ((local_stack || local_scope) && !compile_in_global_scope)
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
    if (token_string_contains_tok(pm->str, len_tok)
        || token_string_contains_tok(pm->str, array_tok))
    {
      tok_str_free(pm->str);
      cprime_free(pm);
      pending_member_funcs[i] = NULL;
      continue;
    }
    rewrite_pending_member_field_access(pm);
    if (guard_non_lifecycle)
      compiling_non_lifecycle_template_member_body++;
    begin_macro(pm->str, 1);
    next();
    if (compile_in_global_scope)
    {
      local_stack = NULL;
      local_scope = 0;
    }
    defer_pending_member_funcs = 1;
    decl(VT_CONST);
    defer_pending_member_funcs = saved_defer_pending_member_funcs;
    /* Each replayed member body is an independent function definition.
       decl() leaves the enclosing local scope live, so restore the saved
       scope before the next queued member is inspected; otherwise a member
       appended mid-flush (e.g. a constructor needed by a static member's
       body) is wrongly deferred forever. */
    local_stack = saved_local_stack;
    local_scope = saved_local_scope;
    end_macro();
    if (guard_non_lifecycle)
      compiling_non_lifecycle_template_member_body--;
    /* The compiled body's token string is freed by end_macro(); clear the
       queue slot so nested scans (pending_member_func_has_body_tok) cannot
       read the dangling token string before the queue is compacted. */
    pending_member_funcs[i] = NULL;
  }
  nb_pending_member_funcs = out;
  local_stack = saved_local_stack;
  local_scope = saved_local_scope;
  defer_pending_member_funcs = saved_defer_pending_member_funcs;
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
  if (tok < TOK_UIDENT)
    cprime_error("namespace name");
  ns_tok = tok;
  note_namespace_tok(ns_tok);
  next();
  if (nb_namespace_stack >= (int)(sizeof(namespace_stack) / sizeof(namespace_stack[0])))
    cprime_error("namespace nesting too deep");
  skip('{');
  namespace_stack[nb_namespace_stack++] = ns_tok;
  while (tok != TOK_EOF && tok != '}')
    decl(VT_CONST);
  skip('}');
  --nb_namespace_stack;
  if (tok == ';')
    next();
}
