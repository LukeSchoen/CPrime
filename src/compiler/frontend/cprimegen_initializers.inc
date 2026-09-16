static void decl_design_delrels(Section *sec, int c, int size)
{
  ObjW_Rel *rel, *rel2, *rel_end;
  if (!sec || !sec->reloc)
    return;
  rel = rel2 = (ObjW_Rel *)sec->reloc->data;
  rel_end = (ObjW_Rel *)(sec->reloc->data + sec->reloc->data_offset);
  while (rel < rel_end)
  {
    if (rel->r_offset >= c && rel->r_offset < c + size)
      sec->reloc->data_offset -= sizeof * rel;
    else
    {
      if (rel2 != rel)
        memcpy(rel2, rel, sizeof *rel);
      ++rel2;
    }
    ++rel;
  }
}

static void decl_design_flex(init_params *p, Sym *ref, int index)
{
  if (!p->flex_array_ref && p->flex_union_fields && ref->c < 0) {
    Sym *field;
    for (field = p->flex_union_fields; field; field = field->next)
      if ((field->type.t & VT_ARRAY) && field->type.ref == ref) {
        p->flex_array_ref = ref;
        break;
      }
  }
  if (ref == p->flex_array_ref)
  {
    if (index >= ref->c)
      ref->c = index + 1;
  }
  else if (ref->c < 0 && index >= 0)
    cprime_error("flexible array has zero size in this context");
}

static void skip_empty_array_fields(Sym **field)
{
  while (*field && ((*field)->type.t & VT_ARRAY)
         && (*field)->type.ref && (*field)->type.ref->c == 0)
    *field = (*field)->next;
}

/* The trailing flexible array member a braced initializer can extend, if the
   aggregate has one.  Unions keep using the constant path's member search. */
static Sym *flexible_array_member(CType *type)
{
  Sym *field;
  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref || IS_UNION(type->t))
    return NULL;
  for (field = type->ref->next; field && field->next; field = field->next)
    ;
  if (field && (field->type.t & VT_ARRAY) && field->type.ref
      && field->type.ref->c < 0)
    return field;
  return NULL;
}

/* Element count the saved braced initializer 'initializer' gives the trailing
   flexible array member of 'type', measured by replaying it in a size-only
   pass.  The member keeps its unknown size afterwards so that every
   declaration derives its own extent.  Stores the member in '*field' and
   returns the count, or -1 when the initializer does not reach the member. */
static int flexible_array_element_count(CType *type, TokenString *initializer,
                                        Sym **field)
{
  init_params probe = {0};
  TokenString macro, *parent;
  const int *parent_ptr;
  int saved_tok = tok, extent = -1;
  CValue saved_tokc = tokc;
  Sym *member = flexible_array_member(type);

  if (field)
    *field = NULL;
  if (!member || !initializer || !saved_initializer_starts_brace(initializer))
    return -1;
  probe.flex_array_ref = member->type.ref;
  parent = macro_stack;
  parent_ptr = macro_ptr;
  macro = *initializer;
  begin_macro(&macro, 0);
  next();
  decl_initializer(&probe, type, 0, DIF_FIRST | DIF_SIZE_ONLY);
  while (macro_stack && macro_stack != parent)
    end_macro();
  macro_ptr = parent_ptr;
  tok = saved_tok;
  tokc = saved_tokc;
  if (probe.flex_array_ref->c >= 0)
    extent = probe.flex_array_ref->c;
  probe.flex_array_ref->c = -1;
  if (field)
    *field = member;
  return extent;
}

/* Reserve room for the 'extent' flexible-array elements a braced initializer
   writes inside the object described by 'p'. */
static void prepare_flexible_array_initializer(init_params *p, Sym *field,
                                               int extent)
{
  int align;
  if (!field || extent <= 0)
    return;
  p->flex_array_ref = field->type.ref;
  p->local_offset += extent * type_size(&field->type.ref->type, &align);
}

/* t is the array or struct type. c is the array or struct
   address. cur_field is the pointer to the current
   field, for arrays the 'c' member contains the current start
   index.  'flags' is as in decl_initializer.
   'al' contains the already initialized length of the
   current container (starting at c).  This returns the new length of that.  */
static int decl_designator(init_params *p, CType *type, unsigned long c,
                           Sym **cur_field, int flags, int al)
{
  Sym *s, *f;
  int index, index_last, align, l, nb_elems, elem_size;
  unsigned long corig = c;

  elem_size = 0;
  nb_elems = 1;

  if (flags & DIF_HAVE_ELEM)
    goto no_designator;

  if (non_iso && tok >= TOK_UIDENT)
  {
    l = tok, next();
    if (tok == ':')
    {
      /* C permits the legacy designated-initializer spelling `field: value`,
         but in C++ the same prefix can begin a qualified expression such as
         `std::move(value)`.  Do not consume the identifier as a field name
         when the colon is followed by another colon. */
      next();
      if (tok != ':')
        goto struct_field_name_read;
      unget_tok(':');
    }
    unget_tok(l);
  }

  // NOTE: we only support ranges for last designator
  while (nb_elems == 1 && (tok == '.'
                           || (tok == '[' && (!is_cpp_translation_unit()
                                               || (type->t & VT_ARRAY)))))
  {
    if (tok == '[')
    {
      if (!(type->t & VT_ARRAY))
        expect("array type");
      next();
      index = index_last = expr_const();
      if (tok == TOK_DOTS && non_iso)
      {
        next();
        index_last = expr_const();
      }
      skip(']');
      s = type->ref;
      decl_design_flex(p, s, index_last);
      if (index < 0 || index_last >= s->c || index_last < index)
        cprime_error("index exceeds array bounds or range is empty");
      if (cur_field)
        (*cur_field)->c = index_last;
      type = pointed_type(type);
      elem_size = type_size(type, &align);
      c += index * elem_size;
      nb_elems = index_last - index + 1;
    }
    else
    {
      int cumofs;
      next();
      l = tok;
struct_field:
      next();
struct_field_name_read:
      f = find_field(type, l, &cumofs);
      if (cur_field)
        *cur_field = f;
      type = &f->type;
      c += cumofs;
    }
    cur_field = NULL;
  }
  if (!cur_field)
  {
    if (tok == '=')
      next();
    else if (!non_iso)
      expect("=");
  }
  else
  {
no_designator:
    if (type->t & VT_ARRAY)
    {
      index = (*cur_field)->c;
      s = type->ref;
      decl_design_flex(p, s, index);
      if (index >= s->c)
        cprime_error("too many initializers");
      type = pointed_type(type);
      elem_size = type_size(type, &align);
      c += index * elem_size;
    }
    else
    {
      f = *cur_field;
      // Skip bitfield padding. Also with size 32 and 64.
      while (f && (f->v & SYM_FIRST_ANOM) &&
             is_integer_btype(f->type.t & VT_BTYPE))
        *cur_field = f = f->next;
      if (!f)
        cprime_error("too many initializers");
      type = &f->type;
      c += f->c;
    }
  }

  if (!elem_size) // For Structs
    elem_size = type_size(type, &align);

  /* Using designators the same element can be initialized more
     than once.  In that case we need to delete possibly already
     existing relocations. */
  if (!(flags & DIF_SIZE_ONLY) && c - corig < al)
  {
    decl_design_delrels(p->sec, c, elem_size *nb_elems);
    flags &= ~DIF_CLEAR; // Mark Stack Dirty Too
  }

  {
    int saved_list = p->list_initialization;
    int saved_member = p->member_token;
    p->list_initialization = 1;
    if (!(type->t & VT_ARRAY) && cur_field && *cur_field)
      p->member_token = (*cur_field)->v & ~SYM_FIELD;
    decl_initializer(p, type, c, flags & ~DIF_FIRST);
    p->member_token = saved_member;
    p->list_initialization = saved_list;
  }

  if (!(flags & DIF_SIZE_ONLY) && nb_elems > 1)
  {
    Sym aref = {0};
    CType t1;
    int i;
    if (p->sec || (type->t & VT_ARRAY))
    {
      // Make Init_Putv/Vstore Believe It Were A Struct
      aref.c = elem_size;
      t1.t = VT_STRUCT, t1.ref = &aref;
      type = &t1;
    }
    if (p->sec)
      vpush_ref(type, p->sec, c, elem_size);
    else
      vset(type, VT_LOCAL | VT_LVAL, c);
    for (i = 1; i < nb_elems; i++)
    {
      vdup();
      init_putv(p, type, c + elem_size *i);
    }
    vpop();
  }

  c += nb_elems * elem_size;
  if (c - corig > al)
    al = c - corig;
  return al;
}

/* Initializer offsets can refer to a stack allocation or to an existing
   object reached through a placement address. */
static void init_push_lvalue(init_params *p, CType *type, unsigned long offset)
{
  if (p->indirect_base)
  {
    vset(&char_pointer_type, VT_LOCAL | VT_LVAL, p->indirect_base);
    if (offset) { vpushi(offset); gen_op('+'); }
    vtop->type = *type;
    mk_pointer(&vtop->type);
    indir();
  }
  else
    vset(type, VT_LOCAL | VT_LVAL, offset);
}

// Store A Value Or An Expression Directly In Global Data Or In Local Array
static void check_scalar_list_initialization(CType *target, SValue *value)
{
  int source_kind = value->type.t & VT_BTYPE, target_kind = target->t & VT_BTYPE;
  int align, source_bits, target_bits, source_unsigned, target_unsigned;
  uint64_t number, maximum;
  if (is_float(target_kind) && is_float(source_kind)
      && ((source_kind == VT_LDOUBLE && target_kind != VT_LDOUBLE)
          || (source_kind == VT_DOUBLE && target_kind == VT_FLOAT))) {
    long double source, converted;
    if ((value->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST)
      cprime_error("narrowing floating-point list initialization");
    source = source_kind == VT_DOUBLE ? value->c.d : value->c.ld;
    if (target_kind == VT_FLOAT) {
      float rounded = (float)source;
      converted = rounded;
    } else {
      double rounded = (double)source;
      converted = rounded;
    }
    if (source - source == 0 && converted - converted != 0)
      cprime_error("out-of-range floating-point list initialization");
    return;
  }
  if (is_float(target_kind) && is_integer_btype(source_kind)) {
    int precision = target_kind == VT_FLOAT ? 24 : 53;
    if ((value->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST)
      cprime_error("narrowing integer-to-floating list initialization");
#if !defined(CPRIME_USING_DOUBLE_FOR_LDOUBLE)
    if (target_kind == VT_LDOUBLE && LDOUBLE_SIZE > 8) precision = 64;
#endif
    source_bits = source_kind == VT_BOOL ? 1 : type_size(&value->type, &align) * 8;
    number = value->c.i;
    if (source_bits < 64) {
      uint64_t mask = (UINT64_C(1) << source_bits) - 1;
      number &= mask;
      if (!(value->type.t & VT_UNSIGNED) && source_kind != VT_BOOL
          && (number & (UINT64_C(1) << (source_bits - 1)))) number |= ~mask;
    }
    if (!(value->type.t & VT_UNSIGNED) && source_kind != VT_BOOL
        && (int64_t)number < 0) number = UINT64_C(0) - number;
    /* Binary floating values represent an integer exactly when its
       significant bits fit after removing trailing powers of two. */
    if (number) while (!(number & 1)) number >>= 1;
    if (precision < 64 && (number >> precision))
      cprime_error("narrowing integer-to-floating list initialization");
    return;
  }
  if (!is_integer_btype(target_kind)) return;
  if (is_float(source_kind)) cprime_error("narrowing floating-to-integer list initialization");
  if (!is_integer_btype(source_kind)) return;
  source_bits = source_kind == VT_BOOL ? 1 : type_size(&value->type, &align) * 8;
  target_bits = target_kind == VT_BOOL ? 1 : type_size(target, &align) * 8;
  source_unsigned = source_kind == VT_BOOL || (value->type.t & VT_UNSIGNED);
  target_unsigned = target_kind == VT_BOOL || (target->t & VT_UNSIGNED);
  if (source_bits < target_bits && (!target_unsigned || source_unsigned)) return;
  if (source_bits <= target_bits && source_unsigned && target_unsigned) return;
  if (source_bits == target_bits && !source_unsigned && !target_unsigned) return;
  if ((value->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST)
    cprime_error("narrowing integer list initialization");
  number = value->c.i;
  if (source_bits < 64)
  {
    uint64_t mask = (UINT64_C(1) << source_bits) - 1;
    number &= mask;
    if (!source_unsigned && (number & (UINT64_C(1) << (source_bits - 1))))
      number |= ~mask;
  }
  if (!source_unsigned && (int64_t)number < 0)
  {
    if (target_unsigned || (target_bits < 64
        && (int64_t)number < -(INT64_C(1) << (target_bits - 1))))
      cprime_error("narrowing integer list initialization");
    return;
  }
  maximum = target_unsigned
    ? (target_bits == 64 ? UINT64_MAX : (UINT64_C(1) << target_bits) - 1)
    : (target_bits == 64 ? INT64_MAX : (UINT64_C(1) << (target_bits - 1)) - 1);
  if (number > maximum) cprime_error("narrowing integer list initialization");
}

static ConstexprLocalField **capture_static_constant_members(
    SValue *source, Sym *destination, int offset, ConstexprLocalField **tail)
{
  Sym *field;
  if ((source->type.t & VT_ARRAY) && !(source->type.t & VT_VLA)
      && source->type.ref) {
    int align, index, size = type_size(&source->type.ref->type, &align);
    if (size <= 0 || source->type.ref->c < 0
        || source->type.ref->c > INT_MAX / size) return tail;
    for (index = 0; index < source->type.ref->c; ++index) {
      SValue child = *source;
      child.type = source->type.ref->type;
      child.type.t |= VT_CONSTANT;
      child.c.i += index * size;
      tail = capture_static_constant_members(&child, destination,
                                             offset + index * size, tail);
    }
    return tail;
  }
  if ((source->type.t & VT_BTYPE) != VT_STRUCT) {
    Sym scalar;
    CValue value;
    Sym *relocation;
    ConstexprPointerBounds pointer_bounds;
    memset(&scalar, 0, sizeof(scalar));
    scalar.type = source->type;
    if (!(scalar.type.t & VT_VOLATILE)
        && constexpr_aggregate_field_value(source, &scalar, 0, &value, &relocation, &pointer_bounds)) {
      ConstexprLocalField *entry = cprime_mallocz(sizeof(*entry));
      entry->object = destination;
      entry->offset = offset;
      entry->type = scalar.type;
      entry->value = value;
      entry->relocation = relocation;
      entry->pointer_bounds = pointer_bounds;
      *tail = entry;
      tail = &entry->next;
    }
    return tail;
  }
  if (!source->type.ref || IS_UNION(source->type.t)) return tail;
  for (field = source->type.ref->next; field; field = field->next) {
    SValue child;
    if (!(field->v & SYM_FIELD) || (field->type.t & (VT_STATIC | VT_VOLATILE))
        || field->a.cpp_mutable_field) continue;
    child = *source;
    child.type = field->type;
    child.type.t |= VT_CONSTANT;
    child.c.i += field->c;
    tail = capture_static_constant_members(&child, destination,
                                           offset + field->c, tail);
  }
  return tail;
}

/* Trivial copies read scalar subobjects. An enclosing mutable or volatile
   subobject cannot supply those reads from an object predating evaluation.
   Union reads additionally need active-member information. */
static int constexpr_copy_has_forbidden_read(CType *type, int forbidden, int allow_mutable)
{
  Sym *field;
  forbidden |= !!(type->t & VT_VOLATILE);
  if (type->t & VT_ARRAY)
    return type->ref && type->ref->c > 0
        && constexpr_copy_has_forbidden_read(&type->ref->type, forbidden, allow_mutable);
  if ((type->t & VT_BTYPE) != VT_STRUCT)
    return forbidden && !is_reference_type(type);
  if (!type->ref || IS_UNION(type->t)) return 0;
  for (field = type->ref->next; field; field = field->next)
    if ((field->v & SYM_FIELD) && !(field->type.t & VT_STATIC)
        && constexpr_copy_has_forbidden_read(&field->type,
             forbidden || (field->a.cpp_mutable_field && !allow_mutable), allow_mutable)) return 1;
  return 0;
}

static void init_putv(init_params *p, CType *type, unsigned long c)
{
  int bt;
  void *ptr;
  CType dtype;
  int size, align;
  Section *sec = p->sec;
  uint64_t val;
  int static_conversion_prev;

  dtype = *type;
  dtype.t &= ~VT_CONSTANT; // need to do that to avoid false warning
  if (p->range_alias_target && !sec)
    cpp_range_note_initializer(p->range_alias_target, vtop);
  p->range_alias_target = NULL;
  if (is_cpp_translation_unit() && (p->copy_list_initialization || p->list_initialization))
    check_scalar_list_initialization(&dtype, vtop);
  if (!is_reference_type(&dtype) && is_reference_type(&vtop->type))
  {
    int category = vtop->type.t & VT_RVALUE_REFERENCE;
    decay_reference_type(&vtop->type);
    indir();
    vtop->type.t |= category;
  }
  if (p->scalar_direct_initialization && (dtype.t & VT_BTYPE) == VT_BOOL
      && (vtop->type.t & VT_BTYPE) == VT_NULLPTR)
    gen_cast(&dtype);
  if (is_reference_type(&dtype))
  {
    CType target = *pointed_type(&dtype);
    int derived_lvalue_to_base = 0;
    /* Binding a reference to a base subobject of a derived object is a
       standard conversion: the reference aliases that subobject.  Running the
       constructor-conversion probe first would slice the object into a
       temporary and bind the reference, and every data pointer read through
       it, to that copy instead. */
    if (!sec
        && (target.t & VT_BTYPE) == VT_STRUCT
        && (vtop->type.t & VT_BTYPE) == VT_STRUCT
        && (vtop->r & VT_LVAL)
        && !is_compatible_unqualified_types(&target, &vtop->type)
        && class_value_is_derived_from(&vtop->type, &target))
      derived_lvalue_to_base = 1;
    if (!derived_lvalue_to_base && !sec && (target.t & VT_BTYPE) == VT_STRUCT
        && ((target.t & VT_CONSTANT) || (dtype.t & VT_RVALUE_REFERENCE))
        && !is_compatible_unqualified_types(&target, &vtop->type))
    {
      target.t &= ~(VT_CONSTANT | VT_VOLATILE);
      try_materialize_constructor_conversion(&target);
    }
    cpp_temp_extend_lifetime(vtop);
    if (!sec && !is_reference_type(&vtop->type)
        && !(vtop->r & VT_LVAL)
        && !(vtop->type.t & (VT_ARRAY | VT_VLA))
        && (vtop->type.t & VT_BTYPE) != VT_FUNC
        && ((dtype.t & VT_RVALUE_REFERENCE)
            || (pointed_type(&dtype)->t & VT_CONSTANT)))
    {
      CType storage_type = *pointed_type(&dtype);
      SValue value;
      int storage_size, storage_align, address;
      storage_type.t &= ~(VT_CONSTANT | VT_VOLATILE);
      gen_assign_cast(&storage_type);
      storage_size = type_size(&storage_type, &storage_align);
      loc = (loc - storage_size) & -storage_align;
      address = loc;
      value = *vtop--;
      vset(&storage_type, VT_LOCAL | VT_LVAL, address);
      vpushv(&value);
      vstore(); vpop();
      vset(&storage_type, VT_LOCAL | VT_LVAL, address);
    }
    decay_reference_type(&dtype);
    if (is_reference_type(&vtop->type))
      decay_reference_type(&vtop->type);
    else
    {
      /* Arrays/functions already carry their address rather than a scalar
         load marker, just as in reference argument binding and unary &. */
      if (!(vtop->type.t & (VT_ARRAY | VT_VLA))
          && (vtop->type.t & VT_BTYPE) != VT_FUNC)
        test_lvalue();
      mk_pointer(&vtop->type);
      gaddrof();
      if (derived_lvalue_to_base)
      {
        CType base_pointer = target;
        base_pointer.t &= ~(VT_CONSTANT | VT_VOLATILE);
        mk_pointer(&base_pointer);
        try_adjust_derived_pointer_to_base(&base_pointer);
      }
    }
    /* An automatic reference whose address is a link-time constant is not
       frame state.  Record the referent so a deferred local class member body
       can keep naming the same object after the frame is gone. */
    if (p->reference_capture_sym && vtop->sym
        && (vtop->r & (VT_VALMASK | VT_LVAL | VT_NONCONST | VT_SYM))
             == (VT_CONST | VT_SYM)
        && (vtop->sym->type.t & VT_BTYPE) != VT_FUNC
        && !(vtop->sym->type.t & VT_TYPEDEF)
        && (vtop->sym->r & VT_VALMASK) == VT_CONST)
    {
      p->reference_capture_sym->lexical_reference_target = vtop->sym;
      p->reference_capture_sym->lexical_reference_offset = (int)vtop->c.i;
      p->reference_capture_sym = NULL;
    }
  }

  size = type_size(type, &align);
  if (type->t & VT_BITFIELD)
    size = (BIT_POS(type->t) + BIT_SIZE(type->t) + 7) / 8;
  init_assert(p, c + size);

  if (p->constexpr_initializer && (dtype.t & VT_BTYPE) == VT_STRUCT
      && vtop->sym && (constexpr_local_object_view(vtop) || (vtop->r & VT_SYM))
      && cpp_type_trivially_copyable(&dtype)
      && is_compatible_unqualified_types(&dtype, &vtop->type)) {
    int active_temporary = is_active_constexpr_temporary(vtop->sym);
    if (constexpr_copy_has_forbidden_read(&vtop->type, 0, active_temporary))
      cprime_error("constexpr copy reads a mutable or volatile subobject");
    if (!vtop->sym->cpp_constexpr_object && !active_temporary
        && constexpr_copy_has_forbidden_read(&vtop->type, 1, 0))
      cprime_error("constexpr copy reads an object not declared constexpr");
  }
  if (p->constant_object_target && (dtype.t & VT_BTYPE) == VT_STRUCT) {
    unsigned bucket = constexpr_local_bucket(p->constant_object_target);
    ConstexprLocalField *copies = NULL, **tail = &copies, *entry;
    int destination = (int)c - p->constant_object_offset;
    if (constexpr_local_object_view(vtop) && cpp_type_trivially_copyable(&dtype)
        && is_compatible_unqualified_types(&dtype, &vtop->type)) {
      int source = (int)vtop->c.i - (int)vtop->sym->c;
      if (vtop->sym == p->constant_object_target && source == destination
          && constexpr_copy_has_forbidden_read(&dtype, 1, 0))
        cprime_error("constexpr initialization copies its own uninitialized storage");
      for (entry = constexpr_local_fields[constexpr_local_bucket(vtop->sym)];
           entry; entry = entry->next) {
        int extent, alignment;
        if (entry->object != vtop->sym) continue;
        extent = entry->zero_size ? (entry->zero_size < 0 ? -entry->zero_size : entry->zero_size)
                                 : type_size(&entry->type, &alignment);
        if (extent <= 0 || entry->offset >= source + size || entry->offset + extent <= source) continue;
        if (!entry->zero_size && (entry->offset < source || entry->offset + extent > source + size)) continue;
        *tail = cprime_malloc(sizeof(**tail));
        **tail = *entry;
        (*tail)->object = p->constant_object_target;
        if (entry->zero_size) {
          int start = entry->offset < source ? source : entry->offset;
          int end = entry->offset + extent > source + size ? source + size : entry->offset + extent;
          (*tail)->offset = destination + start - source;
          (*tail)->zero_size = entry->zero_size < 0 ? start - end : end - start;
        } else (*tail)->offset += destination - source;
        tail = &(*tail)->next;
      }
    }
    if (vtop->sym && (vtop->r & VT_SYM) && (vtop->sym->type.t & VT_CONSTANT)
        && !(vtop->sym->type.t & VT_VOLATILE)
        && cpp_type_trivially_copyable(&dtype)
        && is_compatible_unqualified_types(&dtype, &vtop->type))
      tail = capture_static_constant_members(vtop, p->constant_object_target, destination, tail);
    /* A copy overwrites prior destination initialization even when its
       values cannot yet be evaluated. Do not reuse older zero-fill data. */
    entry = cprime_mallocz(sizeof(*entry));
    entry->object = p->constant_object_target;
    entry->offset = destination;
    entry->zero_size = -size;
    entry->next = constexpr_local_fields[bucket];
    *tail = entry;
    constexpr_local_fields[bucket] = copies;
  }

  if (p->require_constant_values && (dtype.t & VT_BTYPE) != VT_STRUCT) {
    gen_assign_cast(&dtype);
    if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_NONCONST)) != VT_CONST
        && !((dtype.t & VT_BTYPE) == VT_PTR
             && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) == VT_LOCAL
             && is_active_constexpr_temporary(vtop->sym)))
      cprime_error("constexpr variable initializer is not a constant expression");
    if (p->constant_object_target) {
      unsigned bucket = constexpr_local_bucket(p->constant_object_target);
      ConstexprLocalField *entry = cprime_malloc(sizeof(*entry));
      entry->object = p->constant_object_target;
      entry->zero_size = 0;
      entry->offset = (int)c - p->constant_object_offset;
      entry->type = dtype;
      entry->value = vtop->c;
      entry->member_token = p->member_token;
      entry->bit_position = (dtype.t & VT_BITFIELD) ? BIT_POS(dtype.t) : 0;
      if (dtype.t & VT_BITFIELD) {
        entry->value.i = constexpr_bitfield_value(&dtype, entry->value.i);
      }
      entry->relocation = ((vtop->r & VT_SYM) || is_active_constexpr_temporary(vtop->sym)) ? vtop->sym : NULL;
      entry->pointer_bounds = vtop->constexpr_bounds;
      entry->next = constexpr_local_fields[bucket];
      constexpr_local_fields[bucket] = entry;
    }
  }

  if (sec)
  {
    // XXX: not portable
    // XXX: generate error if incorrect relocation
    static_conversion_prev = static_initializer_conversion_wanted;
    ++static_initializer_conversion_wanted;
    gen_assign_cast(&dtype);
    static_initializer_conversion_wanted = static_conversion_prev;
    if (p->pointer_bounds_capture_sym) {
      p->pointer_bounds_capture_sym->constexpr_bounds = vtop->constexpr_bounds;
      p->pointer_bounds_capture_sym = NULL;
    }
    bt = dtype.t &VT_BTYPE;
    if (is_reference_type(type)
        && (vtop->r & (VT_VALMASK | VT_LVAL | VT_NONCONST)) != VT_CONST)
      cprime_error("reference initializer address is not constant");

    /* A complex value is an aggregate of two parts.  Only a value that is
       already an object with known bytes (an anonymous compound literal) can
       be written into a static initializer, so the constant-initialization
       probe fails here for anything else and the declaration takes the
       dynamic-initialization path instead of storing a temporary's offset. */
    if (is_complex_type(&dtype) || is_complex_type(&vtop->type))
    {
      if (is_complex_type(&dtype) && vtop->complex_constant)
      {
        CType element = complex_element_type(&dtype);
        int element_size, element_align;
        SValue value = *vtop;
        element_size = type_size(&element, &element_align);
        vtop--;
        vsetc(&element, VT_CONST, &value.c);
        init_putv(p, &element, c);
        vsetc(&element, VT_CONST, &value.complex_imaginary);
        init_putv(p, &element, c + element_size);
        return;
      }
      if ((vtop->r & (VT_SYM | VT_CONST)) != (VT_SYM | VT_CONST)
          || !vtop->sym || vtop->sym->v < SYM_FIRST_ANOM)
        cprime_error("initializer element is not constant");
    }

    if ((vtop->r & VT_SYM)
        && bt != VT_PTR
        && (bt != (PTR_SIZE == 8 ? VT_LLONG : VT_INT)
            || (type->t & VT_BITFIELD))
        && !((vtop->r & VT_CONST) && (vtop->sym->v >= SYM_FIRST_ANOM
            || (bt == VT_STRUCT && (vtop->sym->type.t & VT_CONSTANT))))
       )
      cprime_error("initializer element is not computable at load time");

    /* A dry run must enforce the same load-time value requirement as data
       emission. A register result from a call is never an initializer byte. */
    if ((vtop->r & (VT_VALMASK | VT_NONCONST)) != VT_CONST
        || ((vtop->r & VT_LVAL)
            && !(bt == VT_STRUCT && (vtop->r & VT_SYM)
                 && vtop->sym
                 && (vtop->sym->v >= SYM_FIRST_ANOM
                     || (vtop->sym->type.t & VT_CONSTANT)))))
      cprime_error("initializer element is not constant");

    if (NODATA_WANTED)
    {
      vtop--;
      return;
    }

    ptr = sec->data + c;
    val = vtop->c.i;

    // XXX: make code faster ?
    if ((vtop->r & (VT_SYM | VT_CONST)) == (VT_SYM | VT_CONST) &&
        (vtop->sym->v >= SYM_FIRST_ANOM
         || (bt == VT_STRUCT && (vtop->sym->type.t & VT_CONSTANT))) &&
        /* XXX This rejects compound literals like
           '(void *){ptr}'.  The problem is that '&sym' is
           represented the same way, which would be ruled out
           by the SYM_FIRST_ANOM check above, but also '"string"'
           in 'char *p = "string"' is represented the same
           with the type being VT_PTR and the symbol being an
           anonymous one.  That is, there's no difference in vtop
           between '(void *){x}' and '&(void *){x}'.  Ignore
           pointer typed entities here.  Hopefully no real code
           will ever use compound literals with scalar type.  */
        (vtop->type.t & VT_BTYPE) != VT_PTR)
    {
      // These come from compound literals, memcpy stuff over.
      Section *ssec;
      ObjSym *esym;
      ObjW_Rel *rel;
      esym = elfsym(vtop->sym);
      ssec = cprime_state->sections[esym->st_shndx];
      memmove (ptr, ssec->data + esym->st_value + (int)vtop->c.i, size);
      if (ssec->reloc)
      {
        /* We need to copy over all memory contents, and that
           includes relocations.  Use the fact that relocs are
           created it order, so look from the end of relocs
           until we hit one before the copied region.  */
        unsigned long relofs = ssec->reloc->data_offset;
        while (relofs >= sizeof(*rel))
        {
          relofs -= sizeof(*rel);
          rel = (ObjW_Rel *)(ssec->reloc->data + relofs);
          if (rel->r_offset >= esym->st_value + size)
            continue;
          if (rel->r_offset < esym->st_value)
            break;
          put_elf_reloca(symtab_section, sec,
                         c + rel->r_offset - esym->st_value,
                         Obj64_R_TYPE(rel->r_info),
                         Obj64_R_SYM(rel->r_info),
#if PTR_SIZE == 8
                         rel->r_addend
#else
                         0
#endif
                        );
        }
      }
    }
    else
    {
      if (type->t & VT_BITFIELD)
      {
        int bit_pos, bit_size, bits, n;
        unsigned char *p, v, m;
        bit_pos = BIT_POS(vtop->type.t);
        bit_size = BIT_SIZE(vtop->type.t);
        p = (unsigned char *)ptr + (bit_pos >> 3);
        bit_pos &= 7, bits = 0;
        while (bit_size)
        {
          n = 8 - bit_pos;
          if (n > bit_size)
            n = bit_size;
          v = val >> bits << bit_pos;
          m = ((1 << n) - 1) << bit_pos;
          *p = (*p & ~m) | (v &m);
          bits += n, bit_size -= n, bit_pos = 0, ++p;
        }
      }
      else
        switch (bt)
        {
        case VT_BOOL:
          *(char *)ptr = val != 0;
          break;
        case VT_BYTE:
          *(char *)ptr = val;
          break;
        case VT_SHORT:
          write16le(ptr, val);
          break;
        case VT_FLOAT:
          write32le(ptr, val);
          break;
        case VT_DOUBLE:
          write64le(ptr, val);
          break;
        case VT_LDOUBLE:
#if defined CPRIME_IS_NATIVE_387
          /* Host and target platform may be different but both have x87.
             On windows, cpc does not use VT_LDOUBLE, except when it is a
             cross compiler.  In this case a mingw gcc as host compiler
             comes here with 10-byte long doubles, while msvc or cpc won't.
             cpc itself can still translate by asm.
             In any case we avoid possibly random bytes 11 and 12.
          */
          if (sizeof (long double) >= 10)
            memcpy(ptr, &vtop->c.ld, 10);
#ifdef __TINYC__
          else if (sizeof (long double) == sizeof (double))
            __asm__("fldl %1\nfstpt %0\n" : "=m" (*ptr) : "m" (vtop->c.ld));
#endif
          else
#endif
            /* For other platforms it should work natively, but may not work
               for cross compilers */
            if (sizeof(long double) == LDOUBLE_SIZE)
              memcpy(ptr, &vtop->c.ld, LDOUBLE_SIZE);
            else if (sizeof(double) == LDOUBLE_SIZE)
              *(double * )ptr = (double)vtop->c.ld;
            else if (0 == memcmp(ptr, &vtop->c.ld, LDOUBLE_SIZE))
              ; // nothing to do for 0.0
#ifndef CPRIME_CROSS_TEST
            else
              cprime_error("can't cross compile long double constants");
#endif
          break;

#if PTR_SIZE == 8
        // Intptr_T May Need A Reloc Too, See Tcctest.C:Relocation_Test()
        case VT_LLONG:
        case VT_PTR:
        case VT_NULLPTR:
          if (vtop->r & VT_SYM)
            greloca(sec, vtop->sym, c, R_DATA_PTR, val);
          else
            write64le(ptr, val);
          break;
        case VT_INT:
          write32le(ptr, val);
          break;
#else
        case VT_LLONG:
          write64le(ptr, val);
          break;
        case VT_PTR:
        case VT_NULLPTR:
        case VT_INT:
          if (vtop->r & VT_SYM)
            greloc(sec, vtop->sym, c, R_DATA_PTR);
          write32le(ptr, val);
          break;
#endif
        default:
          //cprime_internal_error("unexpected type");
          break;
        }
    }
    vtop--;
  }
  else
  {
    if (p->elided_value)
    {
      p->elided_value = 0;
      vpop();
      return;
    }
    if (nocode_wanted && (dtype.t & VT_BTYPE) == VT_STRUCT
        && (vtop->r & VT_CXX_PRVALUE)
        && is_compatible_unqualified_types(&dtype, &vtop->type)) {
      /* Mandatory same-type prvalue initialization has no copy constructor
         to check, including while probing an aggregate field initializer. */
      vpop();
      return;
    }
    if (!p->indirect_base && (dtype.t & VT_BTYPE) == VT_STRUCT
        && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == (VT_LOCAL | VT_LVAL)
        && (int)vtop->c.i == (int)c
        && is_compatible_unqualified_types(&dtype, &vtop->type))
    {
      /* A same-type class prvalue can already occupy the final object. */
      vpop();
      return;
    }
    if ((dtype.t & VT_BTYPE) == VT_STRUCT
        && !is_compatible_unqualified_types(&dtype, &vtop->type))
    {
      int storage = VT_LOCAL | VT_LVAL, address = c;
      if (p->indirect_base) {
        init_push_lvalue(p, &dtype, c);
        mk_pointer(&vtop->type); gaddrof();
        loc = (loc - PTR_SIZE) & -PTR_SIZE;
        address = loc;
        vset(&char_pointer_type, VT_LOCAL | VT_LVAL, address);
        vswap(); vstore(); vpop();
        storage = VT_LLOCAL | VT_LVAL;
      }
      if (try_materialize_constructor_conversion_into(&dtype, 0, storage, address)) {
        vpop();
        return;
      }
      gen_assign_cast(&dtype);
    }
    if ((dtype.t & VT_BTYPE) == VT_STRUCT
        && struct_needs_memberwise_copy(&dtype))
    {
      SValue dst_ptr, src_ptr, source;
      Sym *constructor;
      source = *vtop;
      constructor = resolve_copy_constructor_func(&dtype, &source.type);
      if (constructor && constructor->type.ref && constructor->type.ref->f.func_cpp_explicit) {
        constructor = resolve_implicit_constructor_func(&dtype, &source.type, 1);
        if (!constructor)
          cprime_error("explicit copy constructor is not eligible for copy-initialization");
      }
      vtop--;
      init_push_lvalue(p, &dtype, c);
      mk_pointer(&vtop->type);
      gaddrof();
      dst_ptr = *vtop;
      vtop--;
      vpushv(&source);
      mk_pointer(&vtop->type);
      gaddrof();
      src_ptr = *vtop;
      vtop--;
      if (constructor)
        copy_construct_one_lvalue_pair_ex(&dtype, &dst_ptr, &src_ptr, 0,
          (source.type.t & VT_RVALUE_REFERENCE) != 0, constructor, &source.type);
      else
        copy_construct_struct_memberwise_from_base_ptr(&dtype, &dst_ptr,
                                                       &src_ptr, 0,
          (source.type.t & VT_RVALUE_REFERENCE) != 0);
    }
    else
    {
      SValue object_size_value = *vtop;
      init_push_lvalue(p, &dtype, c);
      vswap();
      vstore();
      vpop();
      object_size_store_value(p->object_size_target, &object_size_value);
    }
  }
}

static int saved_initializer_close(TokenString *expression, int start,
                                    int opening, int closing)
{
  int i, depth = 0;
  for (i = start; i < expression->len; ++i)
  {
    int token = expression->str[i];
    if (TOK_HAS_VALUE(token))
    {
      i += tok_str_value_extra_words(expression->str, expression->len, i);
      continue;
    }
    if (token == opening) ++depth;
    else if (token == closing && !--depth) return i;
  }
  return -1;
}

/* A member call evaluates its receiver subexpression before the final call and
   passes it as the implicit object.  When that receiver group ends in a call
   producing a prvalue of the result type, the receiver's object is a separate
   temporary: the final call still has to construct its own result, so the
   caller's result object must not be forwarded into it. */
static int saved_initializer_receiver_claims_result(TokenString *expression,
                                                     int first, int last,
                                                     CType *type)
{
  TokenString *prefix;
  CType receiver_type;
  int is_prvalue = 0, i;
  if (last < first) return 0;
  prefix = tok_str_alloc();
  for (i = first; i <= last; ++i)
    if (TOK_HAS_VALUE(expression->str[i]))
      tok_str_add_record(prefix, expression->str, expression->len, &i);
    else
      tok_str_add(prefix, expression->str[i]);
  tok_str_add(prefix, TOK_EOF);
  infer_saved_arg_type_and_category(prefix, &receiver_type, &is_prvalue);
  tok_str_free(prefix);
  return is_prvalue && !is_reference_type(&receiver_type)
      && is_compatible_unqualified_types(type, &receiver_type);
}

/* The value produced by an elided initialization must be the destination
   object itself, written through the storage an eliding call was given. */
static int cpp_value_is_construction_destination(SValue *value,
                                                  CppConstructionDestination *destination)
{
  if ((value->type.t & VT_BTYPE) != VT_STRUCT) return 0;
  if ((value->r & (VT_VALMASK | VT_LVAL | VT_SYM)) != destination->storage)
    return 0;
  if ((int)value->c.i != destination->address) return 0;
  return is_compatible_unqualified_types(&value->type, &destination->type);
}

static int saved_initializer_is_direct_constructor(TokenString *expression,
                                                    CType *type)
{
  TokenString *yes = NULL, *no = NULL;
  if (cpp_saved_conditional_at(expression, &yes, &no)) {
    int result = (cpp_saved_arm_is_throw(yes)
                   || saved_initializer_is_direct_constructor(yes, type))
              && (cpp_saved_arm_is_throw(no)
                   || saved_initializer_is_direct_constructor(no, type));
    tok_str_free(yes); tok_str_free(no);
    if (!result)
    {
      CType conditional_type;
      int prvalue = 0;
      infer_saved_arg_type_and_category(expression, &conditional_type, &prvalue);
      result = prvalue && is_compatible_unqualified_types(type, &conditional_type);
    }
    return result;
  }
  int first = tok_str_next_non_linenum_index(expression->str, expression->len, 0);
  int end = tok_str_prev_token_index(expression->str, expression->len,
                                     expression->len);
  int close, next_token;
  if (end >= 0 && expression->str[end] == TOK_EOF)
    end = tok_str_prev_token_index(expression->str, expression->len, end);
  while (first < end && expression->str[first] == '(')
  {
    close = saved_initializer_close(expression, first, '(', ')');
    if (close != end) break;
    first = tok_str_next_non_linenum_index(expression->str, expression->len,
                                           first + 1);
    end = tok_str_prev_token_index(expression->str, expression->len, end);
  }
  if (first >= end) return 0;
  if (expression->str[first] == '(')
  {
    int compound_close = saved_initializer_close(expression, first, '(', ')');
    int compound_next = compound_close < 0 ? -1
      : tok_str_next_non_linenum_index(expression->str, expression->len,
                                       compound_close + 1);
    int compound_type = tok_str_next_non_linenum_index(expression->str,
                                                        expression->len,
                                                        first + 1);
    if (compound_close >= 0 && compound_next <= end
        && expression->str[compound_next] == '{'
        && compound_type <= end
        && class_tok_matches_unqualified_name(get_struct_type_name_tok(type),
                                               expression->str[compound_type]))
      return 1;
  }
  /* A comma expression's result is its final operand's value, so only that
     operand can construct the result object.  Earlier operands are only
     evaluated and discarded. */
  {
    int comma = cpp_expression_top_level_comma(expression);
    if (comma >= 0) {
      int operand = tok_str_next_non_linenum_index(expression->str,
                                                   expression->len, comma + 1);
      if (operand > end) return 0;
      first = operand;
    }
  }
  if (expression->str[first] == '(' || expression->str[first] == '[')
  {
    /* A member call on a computed receiver starts with the receiver group
       `(*p).make()`.  Require it to end before the final call; the scan below
       still validates the receiver and the rest of the expression. */
    int leading_close = saved_initializer_close(expression, first,
        expression->str[first], expression->str[first] == '(' ? ')' : ']');
    int leading_next = leading_close < 0 ? -1
        : tok_str_next_non_linenum_index(expression->str, expression->len,
                                         leading_close + 1);
    if (leading_close < 0 || leading_close >= end || leading_next > end
        || (expression->str[leading_next] != '.'
            && expression->str[leading_next] != TOK_ARROW))
      return 0;
  }
  else if ((expression->str[first] < TOK_UIDENT && expression->str[first] != ':'
            && expression->str[first] != TOK_CO_AWAIT)
      || TOK_HAS_VALUE(expression->str[first]))
    return 0;
  if (is_cpp_lambda_marker(expression->str[first]))
  {
    int i, capture_depth = 1;
    for (i = first + 1; i <= end; ++i)
    {
      int token = expression->str[i];
      if (TOK_HAS_VALUE(token))
      {
        i += tok_str_value_extra_words(expression->str, expression->len, i);
        continue;
      }
      if (capture_depth) {
        if (token == '[' || is_cpp_lambda_marker(token)) ++capture_depth;
        else if (token == ']') --capture_depth;
        continue;
      }
      if (token == '{')
      {
        CType closure_type;
        if (saved_initializer_close(expression, i, '{', '}') != end) return 0;
        infer_saved_arg_type(expression, &closure_type);
        return is_compatible_unqualified_types(type, &closure_type);
      }
    }
    return 0;
  }
  next_token = tok_str_next_non_linenum_index(expression->str, expression->len,
                                              first + 1);
  if (next_token <= end && expression->str[next_token] == TOK_LT
      && find_class_template_def(find_current_namespace_tok(expression->str[first])))
  {
    int i, depth = 0, parentheses = 0, type_end = -1;
    TokenString *type_tokens, *parent;
    const int *parent_ptr;
    CType constructed_type;
    AttributeDef ad;
    int saved_tok, unused_name = 0, parsed;
    CValue saved_tokc;
    for (i = next_token; i <= end; ++i)
    {
      int token = expression->str[i];
      if (TOK_HAS_VALUE(token))
      {
        i += tok_str_value_extra_words(expression->str, expression->len, i);
        continue;
      }
      if (token == '(' || token == '[') ++parentheses;
      else if (token == ')' || token == ']') --parentheses;
      else if (!parentheses)
      {
        if (token == TOK_LT) ++depth;
        else if (token == TOK_GT) --depth;
        else if (token == TOK_SAR) depth -= 2;
        if (!depth) { type_end = i; break; }
      }
    }
    if (type_end < 0) return 0;
    next_token = tok_str_next_non_linenum_index(expression->str, expression->len,
                                                type_end + 1);
    if (next_token > end || expression->str[next_token] != '('
        || saved_initializer_close(expression, next_token, '(', ')') != end)
      return 0;
    type_tokens = tok_str_alloc();
    for (i = first; i <= type_end; ++i)
      if (TOK_HAS_VALUE(expression->str[i]))
        tok_str_add_record(type_tokens, expression->str, expression->len, &i);
      else tok_str_add(type_tokens, expression->str[i]);
    tok_str_add(type_tokens, TOK_EOF);
    saved_tok = tok;
    saved_tokc = tokc;
    parent = macro_stack;
    parent_ptr = macro_ptr;
    memset(&ad, 0, sizeof(ad));
    begin_macro(type_tokens, 1);
    next();
    parsed = parse_btype(&constructed_type, &ad, 0);
    if (parsed) type_decl(&constructed_type, &ad, &unused_name, TYPE_ABSTRACT);
    while (macro_stack && macro_stack != parent) end_macro();
    macro_ptr = parent_ptr;
    tok = saved_tok;
    tokc = saved_tokc;
    return parsed && is_compatible_unqualified_types(type, &constructed_type);
  }
  if (class_tok_matches_unqualified_name(get_struct_type_name_tok(type),
                                          expression->str[first])
      && next_token <= end && expression->str[next_token] == '('
      && saved_initializer_close(expression, next_token, '(', ')') == end)
    return 1;
  {
    int index, angles = 0, call = -1, is_class_prvalue;
    CType result;
    /* A single call can have a qualified/member name and explicit template
       arguments.  A member call may also have a receiver subexpression, as in
       `(*p).make()` or `A().make()`; those groups are skipped so the final call
       still claims the result object.  That receiver is evaluated first and
       becomes the implicit object of the call, so a receiver that is itself a
       prvalue of the result type is a distinct temporary and blocks the claim
       (`make().convert()` must not give make() the final result object).
       Operators still reject the claim conservatively. */
    for (index = first; index <= end; ++index) {
      int token = expression->str[index];
      if (index == first && token == TOK_CO_AWAIT) continue;
      if (TOK_HAS_VALUE(token)) {
        if (!angles && token != TOK_LINENUM) return 0;
        index += tok_str_value_extra_words(expression->str, expression->len, index);
        continue;
      }
      if (!angles && (token == '(' || token == '[' || token == '{'))
      {
        int close, next;
        close = saved_initializer_close(expression, index, token,
                                        token == '(' ? ')' : token == '[' ? ']' : '}');
        if (close < 0) return 0;
        if (close == end)
        {
          if (token == '[') return 0;   /* a subscript is not a prvalue construction */
          call = index;
          break;
        }
        next = tok_str_next_non_linenum_index(expression->str, expression->len,
                                              close + 1);
        if (next > end
            || (expression->str[next] != '.'
                && expression->str[next] != TOK_ARROW))
          return 0;
        if (saved_initializer_receiver_claims_result(expression, first, close,
                                                     type))
          return 0;
        index = close;
        continue;
      }
      if (token == '<' || token == TOK_LT) ++angles;
      else if (token == TOK_GT && angles) --angles;
      else if ((token == TOK_SAR || token == TOK_SHR) && angles)
        angles = angles > 1 ? angles - 2 : 0;
      else if (!angles && token < TOK_UIDENT && token != ':'
               && token != '.' && token != TOK_ARROW) return 0;
    }
    if (call < 0 || saved_initializer_close(expression, call,
          expression->str[call], expression->str[call] == '{' ? '}' : ')') != end)
      return 0;
    infer_saved_arg_type_and_category(expression, &result, &is_class_prvalue);
    if (!is_class_prvalue || is_reference_type(&result)) return 0;
    result.t &= ~VT_RVALUE_REFERENCE;
    return is_compatible_unqualified_types(type, &result);
  }
}

static int emit_class_initializer_expression_at_ex(TokenString *expression,
                                                 CType *type,
                                                 unsigned long address,
                                                 int storage, int extend_backing)
{
  TokenString macro, *parent;
  const int *parent_ptr;
  CppConstructionDestination destination, *saved_destination;
  int saved_tok, forwarding;
  CValue saved_tokc;
  prepare_saved_cpp_lambdas(expression);
  saved_tok = tok;
  saved_tokc = tokc;
  parent = macro_stack;
  parent_ptr = macro_ptr;
  saved_destination = cpp_construction_destination;
  destination.type = *type;
  destination.address = address;
  destination.storage = storage;
  destination.available = saved_initializer_is_direct_constructor(expression, type);
  forwarding = destination.available;
  destination.conditional_marker = cpp_prepare_expression_destination(expression);
  destination.extend_backing_array_lifetime = extend_backing;
  cpp_construction_destination = &destination;
  macro = *expression;
  begin_macro(&macro, 0);
  next();
  parse_init_elem(EXPR_ANY);
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr;
  cpp_construction_destination = saved_destination;
  tok = saved_tok;
  tokc = saved_tokc;
  /* The destination can be claimed by an inner call whose object is not the
     expression's own result value (`(make()).convert()`). Report elision only
     when the value produced for this initializer is the destination object. */
  return forwarding && !destination.available
      && cpp_value_is_construction_destination(vtop, &destination);
}

static int emit_class_initializer_expression_at(TokenString *expression,
                                                 CType *type, unsigned long address,
                                                 int storage)
{
  return emit_class_initializer_expression_at_ex(expression, type, address, storage, 0);
}

static void emit_class_initializer_expression(TokenString *expression,
                                               CType *type,
                                               unsigned long address)
{
  emit_class_initializer_expression_at_ex(expression, type, address,
                                        VT_LOCAL | VT_LVAL, 1);
}

static void initialize_lifecycle_copy(CType *type, int r, int addr,
                                      TokenString *expression)
{
  init_params p = {0};
  int align, size = type_size(type, &align);
  /* A named return object can live behind the caller's result pointer.
     Its frame slot stores an address, not the object's bytes. */
  p.indirect_base = (r & VT_VALMASK) == VT_LLOCAL ? addr : 0;
  p.local_offset = (p.indirect_base ? 0 : addr) + size;
  if (expression)
    p.elided_value = emit_class_initializer_expression_at(expression, type,
                                                          addr, r);
  init_putv(&p, type, p.indirect_base ? 0 : addr);
}

static int parse_direct_class_initializer(init_params *p, CType *type,
                                           unsigned long address)
{
  TokenString *expression;
  if (tok != '(' && tok != '[' && tok != TOK_CO_AWAIT && tok < TOK_UIDENT)
    return 0;
  /* Inspect the complete expression before forwarding storage. For example,
     in T value = T(1) + T(2), neither operand is value's result object. */
  skip_or_save_block_ex(&expression, 1);
  if (p->indirect_base)
  {
    int slot;
    init_push_lvalue(p, type, address);
    mk_pointer(&vtop->type); gaddrof();
    vtop->type = char_pointer_type;
    loc = (loc - PTR_SIZE) & -PTR_SIZE;
    slot = loc;
    vset(&char_pointer_type, VT_LOCAL | VT_LVAL, slot);
    vswap(); vstore(); vpop();
    p->elided_value = emit_class_initializer_expression_at_ex(expression, type,
                         slot, VT_LLOCAL | VT_LVAL, p->extend_backing_array_lifetime);
  }
  else
    p->elided_value = emit_class_initializer_expression_at_ex(expression, type, address,
                         VT_LOCAL | VT_LVAL, p->extend_backing_array_lifetime);
  tok_str_free(expression);
  return 1;
}

static void restore_initializer_cleanups(Sym *stop, int depth)
{
  Sym *cleanup, **temporaries = NULL;
  int count = 0, i, j;
  for (cleanup = cur_scope->cl.s; cleanup && cleanup != stop; cleanup = cleanup->next)
    if (cleanup->a.cleanup_temporary && cleanup->cleanup_func)
      dynarray_add(&temporaries, &count, cleanup);
  cur_scope->cl.s = stop;
  cur_scope->cl.n = depth;
  for (i = count - 1; i >= 0; --i)
  {
    Sym *old = temporaries[i];
    Sym *copy = sym_push2(&all_cleanups, SYM_FIELD | ++cur_scope->cl.n, 0, 0);
    copy->cleanup_sym = old->cleanup_sym;
    copy->cleanup_func = old->cleanup_func;
    copy->a = old->a;
    copy->const_value = old->const_value;
    copy->next = cur_scope->cl.s;
    cur_scope->cl.s = copy;
    /* Keep earlier unwind/goto chains intact while the new complete-object
       chain retains the full-expression's still-live argument temporaries. */
    for (j = 0; j < cpp_temporary_count; ++j)
      if (cpp_temporaries[j].cleanup == old) cpp_temporaries[j].cleanup = copy;
  }
  cprime_free(temporaries);
}

static void emit_placement_aggregate_initializer(CType *type, TokenString *placement,
                                                 TokenString *initializer)
{
  init_params p = {0};
  TokenString macro = *initializer, *parent;
  const int *parent_ptr;
  int saved_tok = tok, alignment;
  CValue saved_tokc = tokc;
  Sym *old_cleanup = cur_scope->cl.s;
  Sym *flex_field = NULL;
  int old_cleanup_depth = cur_scope->cl.n;
  int flex_extent;
  CType pointer = *type;
  /* A braced initializer fixes the extent of a trailing flexible array
     member.  The declaration reserved that many elements already. */
  flex_extent = flexible_array_element_count(type, initializer, &flex_field);
  mk_pointer(&pointer);
  replay_saved_placement_expr(placement);
  gen_cast(&pointer);
  loc = (loc - PTR_SIZE) & -PTR_SIZE;
  p.indirect_base = loc;
  p.local_offset = type_size(type, &alignment);
  p.track_partial_cleanups = 1;
  prepare_flexible_array_initializer(&p, flex_field, flex_extent);
  vdup();
  vset(&pointer, VT_LOCAL | VT_LVAL, p.indirect_base);
  vswap(); vstore(); vpop();
  parent = macro_stack;
  parent_ptr = macro_ptr;
  begin_macro(&macro, 0);
  next();
  decl_initializer(&p, type, 0, DIF_FIRST);
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr;
  if (flex_field)
    flex_field->type.ref->c = -1;
  restore_initializer_cleanups(old_cleanup, old_cleanup_depth);
  tok = saved_tok; tokc = saved_tokc;
}

static int initialize_class_constructor_list(init_params *p, CType *type,
                                               unsigned long address, int flags)
{
  TokenString *initializer, **arguments;
  CType actual[32], target = *type;
  MemberFuncOverload *candidate;
  Sym *constructor = NULL;
  int count, i, best_rank = 0x7fffffff, ambiguous = 0;
  int class_tok = get_struct_type_name_tok(type), is_list = 0, implicit_copy = 0;
  int elided_prvalue = 0;
  if (tok != '{' || p->sec || (flags & DIF_SIZE_ONLY)
      || !is_cpp_translation_unit() || !type->ref
      || !type->ref->a.cpp_user_provided_constructor
      || type_is_std_initializer_list(type))
    return 0;
  target.t &= ~(VT_CONSTANT | VT_VOLATILE);
  skip_or_save_block(&initializer);
  count = split_saved_array_initializer(initializer, &arguments);
  if (count > 32) cprime_error("too many constructor arguments");
  if (count) {
    constructor = resolve_initializer_list_constructor(&target);
    if (constructor) {
      Sym *parameter = constructor->type.ref->next;
      if (parameter) parameter = parameter->next;
      if (!parameter || saved_list_argument_rank(&parameter->type, initializer) < 0)
        constructor = NULL;
      else is_list = 1;
    }
  }
  if (!constructor) {
    infer_saved_arg_types(arguments, actual, count);
    if (count == 1 && !saved_arg_is_braced(arguments[0])) {
      CType source = actual[0];
      if (is_reference_type(&source)) source = *pointed_type(&source);
      source.t &= ~VT_RVALUE_REFERENCE;
      if (is_compatible_unqualified_types(&target, &source)) {
        CType inferred;
        infer_saved_arg_type_and_category(arguments[0], &inferred, &elided_prvalue);
        if (!elided_prvalue)
          constructor = resolve_copy_constructor_func(&target, &actual[0]);
        implicit_copy = !constructor;
      }
    }
    if (!implicit_copy)
      instantiate_template_member_for_call(&target, TOK_CONSTRUCTOR1, actual, count);
    for (candidate = implicit_copy ? NULL : member_func_candidates(class_tok, TOK_CONSTRUCTOR1);
         candidate; candidate = candidate->bucket_next) {
      Sym *function, *parameter;
      int rank = 0;
      if (candidate->struct_tok != class_tok
          || candidate->method_tok != TOK_CONSTRUCTOR1
          || !member_overload_accepts_argument_count(candidate, count))
        continue;
      function = find_global_symbol(candidate->mangled_tok);
      if (!function || !function->type.ref) continue;
      use_overload_func_type(function, &candidate->func_type);
      parameter = function->type.ref->next;
      if (parameter) parameter = parameter->next;
      for (i = 0; i < count && parameter; ++i, parameter = parameter->next) {
        int conversion = saved_arg_is_braced(arguments[i])
          ? saved_list_argument_rank(&parameter->type, arguments[i])
          : call_arg_match_rank(&parameter->type, &actual[i]);
        if (conversion < 0) break;
        rank += conversion;
      }
      if (i != count) continue;
      if (rank > best_rank) continue;
      if (rank == best_rank && constructor && constructor != function) {
        int order = compare_member_function_template_order(function->v, constructor->v);
        if (order < 0) continue;
        if (!order) { ambiguous = 1; continue; }
      }
      constructor = function; best_rank = rank; ambiguous = 0;
    }
  }
  if (!constructor && !implicit_copy && !count) {
    constructor = resolve_initializer_list_constructor(&target);
    is_list = constructor != NULL;
  }
  if (ambiguous || (!constructor && !implicit_copy)
      || (constructor && p->copy_list_initialization
          && constructor->type.ref->f.func_cpp_explicit)) {
    free_saved_array_elements(arguments, count);
    tok_str_free(initializer);
    if (ambiguous) cprime_error("ambiguous constructor in list-initialization");
    if (!constructor) cprime_error("no matching constructor for '%s' with %d list-initializer elements",
                                   get_tok_str(class_tok, NULL), count);
    cprime_error("copy-list-initialization selects an explicit constructor");
  }
  if ((!nocode_wanted || constexpr_pointer_evaluation_active()) && implicit_copy) {
    TokenString replay = *arguments[0], *parent = macro_stack;
    const int *parent_ptr = macro_ptr;
    int saved_tok = tok;
    CValue saved_tokc = tokc;
    begin_macro(&replay, 0); next();
    if (!elided_prvalue || !parse_direct_class_initializer(p, &target, address))
      parse_init_elem(EXPR_ANY);
    init_putv(p, &target, address);
    while (macro_stack && macro_stack != parent) end_macro();
    macro_ptr = parent_ptr; tok = saved_tok; tokc = saved_tokc;
  } else if (!nocode_wanted || constexpr_pointer_evaluation_active()) {
    int storage = VT_LOCAL | VT_LVAL, location = address;
    Sym *destination_object = NULL;
    if (p->indirect_base) {
      init_push_lvalue(p, &target, address);
      mk_pointer(&vtop->type); gaddrof();
      vtop->type = char_pointer_type;
      loc = (loc - PTR_SIZE) & -PTR_SIZE;
      location = loc;
      vset(&char_pointer_type, VT_LOCAL | VT_LVAL, location);
      vswap(); vstore(); vpop();
      storage = VT_LLOCAL | VT_LVAL;
    } else if (p->constant_object_target
               && p->constant_object_offset == (int)address) {
      /* The evaluation owns this object: its constructor writes the
         subobjects into the evaluation record. */
      destination_object = p->constant_object_target;
    }
    emit_lifecycle_constructor_selected(&target, storage, location, VT_CONST,
        destination_object,
        constructor, is_list ? &initializer : arguments, is_list ? 1 : count);
  }
  free_saved_array_elements(arguments, count);
  tok_str_free(initializer);
  return 1;
}

/* A block-scope `const int` object is a constant expression once its
   initializer is one, so a reference to another integral constant still
   supplies a value even though the initializer itself is evaluated as an
   ordinary (non-constant) expression context. */
static int initializer_integral_constant_value(SValue *value, int64_t *result)
{
  Sym *symbol = value->sym;
  int kind = value->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST);

  if (kind == VT_CONST && is_integer_btype(value->type.t & VT_BTYPE))
  {
    *result = value->c.i;
    return 1;
  }
  if (kind != (VT_CONST | VT_SYM) && kind != (VT_LOCAL | VT_LVAL))
    return 0;
  if (!symbol || !symbol->a.integral_constexpr
      || !(symbol->type.t & VT_CONSTANT)
      || !is_integer_btype(symbol->type.t & VT_BTYPE))
    return 0;
  *result = symbol->const_value;
  return 1;
}

/* Default member initializers are stored as source tokens.  When an omitted
   member is replayed as part of an aggregate initializer, make references to
   the aggregate's non-static fields explicit on the object being initialized.
   This preserves ordinary lookup for names that are not fields (types,
   functions, namespace constants, and qualified members). */
static TokenString *bind_aggregate_default_initializer(TokenString *source,
                                                        CType *aggregate)
{
  TokenString *bound = tok_str_alloc();
  int i, previous = 0;

  for (i = 0; i < source->len; )
  {
    int current = source->str[i];
    if (current >= TOK_UIDENT && previous != '.' && previous != TOK_ARROW
        && previous != ':')
    {
      int offset;
      Sym *field = find_field_try(aggregate, current, &offset);
      if (field && !(field->type.t & (VT_TYPEDEF | VT_STATIC))
          && (field->type.t & VT_BTYPE) != VT_FUNC)
      {
        tok_str_add(bound, TOK_CXX_INITIALIZER_OBJECT);
        tok_str_add(bound, '.');
      }
    }
    if (TOK_HAS_VALUE(current))
    {
      tok_str_add_record(bound, source->str, source->len, &i);
      ++i;
    }
    else
    {
      tok_str_add(bound, current);
      ++i;
    }
    previous = current;
  }
  return bound;
}

/* 't' contains the type and storage info. 'c' is the offset of the
   object in section 'sec'. If 'sec' is NULL, it means stack based
   allocation. 'flags & DIF_FIRST' is true if array '{' must be read (multi
   dimension implicit array init handling). 'flags & DIF_SIZE_ONLY' is true if
   size only evaluation is wanted (only for arrays). */
static void decl_initializer_impl(init_params *p, CType *type, unsigned long c, int flags);

static void decl_initializer(init_params *p, CType *type, unsigned long c, int flags)
{
  long long saved_frontier = constant_initialization_frontier;
  /* C++ aggregate subobjects are initialized in declaration/index order.
     Pre-cleared output bytes beyond this subobject are not initialized values. */
  if ((p->sec || p->constant_object_target) && constant_initialization_object)
    constant_initialization_frontier = p->sec ? (long long)c : (int)c;
  decl_initializer_impl(p, type, c, flags);
  constant_initialization_frontier = saved_frontier;
}

static void decl_initializer_impl(init_params *p, CType *type, unsigned long c, int flags)
{
  int single_expression_list = 0;
  int len, n, no_oblock, i;
  int size1, align1;
  Sym *s, *f;
  Sym indexsym;
  CType *t1;
  Sym *old_cleanup = cur_scope ? cur_scope->cl.s : NULL;
  int old_cleanup_depth = cur_scope ? cur_scope->cl.n : 0;

  // Generate Line Number Info
  if (debug_modes && !(flags & DIF_SIZE_ONLY) && !p->sec)
    cprime_debug_line(cprime_state), cprime_tcov_check_line (cprime_state, 1);

  if (tok == '{' && !(flags & DIF_HAVE_ELEM) && is_cpp_translation_unit()
      && (type->t & VT_BTYPE) == VT_STRUCT && !(type->t & VT_ARRAY)
      && constexpr_aggregate_temporary_type(type) && cpp_type_trivially_copyable(type)) {
    TokenString *initializer, **elements = NULL;
    int count;
    skip_or_save_block(&initializer);
    count = split_saved_array_initializer(initializer, &elements);
    if (count == 1) {
      int first = tok_str_next_non_linenum_index(elements[0]->str, elements[0]->len, 0);
      int second = tok_str_next_non_linenum_index(elements[0]->str, elements[0]->len, first + 1);
      single_expression_list = elements[0]->str[first] != '.' && elements[0]->str[first] != '{'
          && !(second < elements[0]->len && elements[0]->str[second] == ':');
    }
    free_saved_array_elements(elements, count);
    {
      TokenString *replay = tok_str_alloc();
      tok_str_append_without_eof(replay, initializer);
      tok_str_free(initializer);
      restore_cpp_lifecycle_probe(replay);
    }
  }
  if (!(flags & DIF_HAVE_ELEM) && tok != '{' &&
      /* In case of strings we have special handling for arrays, so
         don't consume them as initializer value (which would commit them
         to some anonymous symbol).  */
      ((!TOK_IS_STRING(tok)) || !(type->t & VT_ARRAY)) &&
      (!(flags & DIF_SIZE_ONLY)
       /* a struct may be initialized from a struct of same type, as in
               struct {int x,y;} a = {1,2}, b = {3,4}, c[] = {a,b};
          In that case we need to parse the element in order to check
          it for compatibility below */
       || (type->t & VT_BTYPE) == VT_STRUCT)
     )
  {
    int ncw_prev = nocode_wanted;
    CType *saved_member_target = cpp_member_address_target;
    int reference_fold_prev = suppress_integral_constexpr_fold;
    /* Binding a reference keeps the referent's identity: the initializer has
       to stay an lvalue even when a constant evaluator could read a value
       out of it, exactly as for a reference declaration. */
    if (is_reference_type(type)) ++suppress_integral_constexpr_fold;
    cpp_member_address_target = find_cpp_member_pointer_type(type) ? type : NULL;
    if ((flags & DIF_SIZE_ONLY) && !p->sec)
      ++nocode_wanted;
    if (!(!p->sec && !(flags & DIF_SIZE_ONLY)
          && is_cpp_translation_unit() && (type->t & VT_BTYPE) == VT_STRUCT
          && parse_direct_class_initializer(p, type, c)))
      parse_init_elem(!p->sec || is_reference_type(type) || static_initializer_constant_fold ? EXPR_ANY : EXPR_CONST);
    suppress_integral_constexpr_fold = reference_fold_prev;
    cpp_member_address_target = saved_member_target;
    nocode_wanted = ncw_prev;
    flags |= DIF_HAVE_ELEM;
  }

  if (find_cpp_member_pointer_type(type))
  {
    if (!(flags & DIF_HAVE_ELEM))
    {
      skip('{');
      if (tok == '}')
        vpushi(0);
      else
        parse_init_elem(!p->sec ? EXPR_ANY : EXPR_CONST);
      if (tok == ',') next();
      skip('}');
    }
    if (flags & DIF_SIZE_ONLY)
      vpop();
    else
    {
      convert_cpp_member_pointer(type, 0);
      init_putv(p, type, c);
    }
    return;
  }

  if (type->t & VT_ARRAY)
  {
    no_oblock = 1;
    if (((flags & DIF_FIRST) && !TOK_IS_STRING(tok)) ||
        tok == '{')
    {
      skip('{');
      no_oblock = 0;
    }

    s = type->ref;
    n = s->c;
    t1 = pointed_type(type);
    size1 = type_size(t1, &align1);

    /* only parse strings here if correct type (otherwise: handle
       them as ((w)char *) expressions */
    if ((tok == TOK_U16STR && (t1->t & ~(VT_STORAGE | VT_CONSTANT | VT_VOLATILE)) == VT_CHAR16_T)
        || (tok == TOK_U32STR && (t1->t & ~(VT_STORAGE | VT_CONSTANT | VT_VOLATILE)) == VT_CHAR32_T)
        || (tok == TOK_LSTR &&
#ifdef CPRIME_TARGET_PE
         (t1->t & VT_BTYPE) == VT_SHORT && (t1->t & VT_UNSIGNED)
#else
         (t1->t & VT_BTYPE) == VT_INT
#endif
        ) || ((tok == TOK_STR || tok == TOK_U8STR) && (t1->t & VT_BTYPE) == VT_BYTE))
    {
      len = 0;
      cstr_reset(&initstr);
      int string_token = tok;
      if (size1 != TOK_STRING_UNIT_SIZE(tok))
        cprime_error("unhandled string literal merging");
      while (TOK_IS_STRING(tok))
      {
        if (tok != string_token) cprime_error("incompatible string literal encodings");
        if (initstr.size)
          initstr.size -= size1;
        if (tok == TOK_STR || tok == TOK_U8STR)
          len += tokc.str.size;
        else
          len += tokc.str.size / TOK_STRING_UNIT_SIZE(tok);
        len--;
        cstr_cat(&initstr, tokc.str.data, tokc.str.size);
        next();
      }
      if (tok != ')' && tok != '}' && tok != ',' && tok != ';'
          && tok != TOK_EOF)
      {
        // Not a lone literal but part of a bigger expression.
        unget_tok(string_token);
        tokc.str.size = initstr.size;
        tokc.str.data = initstr.data;
        goto do_init_array;
      }

      decl_design_flex(p, s, len);
      /* A constant-initialization probe can infer an incomplete array bound
         in this same pass. Use the completed bound when checking/copying. */
      n = s->c;
      if (!(flags & DIF_SIZE_ONLY))
      {
        int nb = n, ch;
        if (len < nb)
          nb = len;
        if (len > nb)
          cprime_warning("initializer-string for array is too long");
        /* in order to go faster for common case (char
           string in global variable, we handle it
           specifically */
        if (p->sec && size1 == 1)
        {
          init_assert(p, c + nb);
          if (!NODATA_WANTED)
            memcpy(p->sec->data + c, initstr.data, nb);
        }
        else
        {
          for (i = 0; i < n; i++)
          {
            if (i >= nb)
            {
              /* only add trailing zero if enough storage (no
                 warning in this case since it is standard) */
              if (flags & DIF_CLEAR)
                break;
              if (n - i >= 4)
              {
                init_putz(p, c + i *size1, (n - i) * size1);
                break;
              }
              ch = 0;
            }
            else if (size1 == 1)
              ch = ((unsigned char *)initstr.data)[i];
            else
              ch = size1 == 2 ? ((uint16_t *)initstr.data)[i] : ((uint32_t *)initstr.data)[i];
            vpushi(ch);
            init_putv(p, t1, c + i *size1);
          }
        }
      }
    }
    else
    {

do_init_array:
      indexsym.c = 0;
      f = &indexsym;

do_init_list:
      // Zero Memory Once In Advance
      if (!(flags & (DIF_CLEAR | DIF_SIZE_ONLY)))
      {
        init_putz(p, c, n *size1);
        flags |= DIF_CLEAR;
      }

      /* Evaluate at the normal element position, after destination setup.
         The lexical scan above neither evaluates nor declares anything. */
      if (single_expression_list) {
        int reference_first_member = 0;
        single_expression_list = 0;
        /* The element of a one-item list that initializes a reference member
           binds that reference, so it must keep the referent's identity. */
        if ((type->t & VT_BTYPE) == VT_STRUCT && type->ref) {
          Sym *first_field = type->ref->next;
          skip_empty_array_fields(&first_field);
          reference_first_member = first_field && is_reference_type(&first_field->type);
        }
        if (reference_first_member) ++suppress_integral_constexpr_fold;
        parse_init_elem(!p->sec || static_initializer_constant_fold ? EXPR_ANY : EXPR_CONST);
        if (reference_first_member) --suppress_integral_constexpr_fold;
        if (is_compatible_unqualified_types(type, &vtop->type)) {
          if (flags & DIF_SIZE_ONLY) vpop();
          else init_putv(p, type, c);
          if (tok == ',') next();
          skip('}');
          return;
        }
        flags |= DIF_HAVE_ELEM;
      }
      len = 0;
      /* language extension: if the initializer is empty for a flex array,
         it's size is zero.  We won't enter the loop, so set the size
         now.  */
      decl_design_flex(p, s, len - 1);
      while (tok != '}' || (flags & DIF_HAVE_ELEM))
      {
        len = decl_designator(p, type, c, &f, flags, len);
        flags &= ~DIF_HAVE_ELEM;
        if (type->t & VT_ARRAY)
        {
          ++indexsym.c;
          /* special test for multi dimensional arrays (may not
             be strictly correct if designators are used at the
             same time) */
          if (no_oblock && len >= n * size1)
            break;
        }
        else
        {
          if (s->type.t == VT_UNION)
            f = NULL;
          else
          {
            f = f->next;
            skip_empty_array_fields(&f);
          }
          if (no_oblock && f == NULL)
            break;
        }

        if (tok == '}')
          break;
        skip(',');
      }
    }
    if ((type->t & VT_BTYPE) == VT_STRUCT && !IS_UNION(type->t)
        && !(flags & DIF_SIZE_ONLY) && is_cpp_translation_unit()) {
      for (; f; f = f->next) {
        TokenString *initializer;
        TokenString *parent;
        const int *parent_ptr;
        int saved_tok;
        int saved_list = p->list_initialization;
        int saved_copy_list = p->copy_list_initialization;
        CValue saved_tokc;
        SValue saved_receiver = cpp_default_initializer_receiver;
        int saved_receiver_offset = cpp_default_initializer_receiver_offset;
        if ((f->type.t & VT_BTYPE) != VT_STRUCT && !(f->type.t & VT_ARRAY)
            && !f->default_arg) continue;
        initializer = f->default_arg && p->aggregate_target_sym
          ? bind_aggregate_default_initializer(f->default_arg, type)
          : tok_str_alloc();
        if (f->default_arg && !p->aggregate_target_sym)
          tok_str_append_without_eof(initializer, f->default_arg);
        else if (!f->default_arg) { tok_str_add(initializer, '{'); tok_str_add(initializer, '}'); }
        tok_str_add(initializer, tok); tok_str_add(initializer, 0);
        parent = macro_stack; parent_ptr = macro_ptr;
        saved_tok = tok; saved_tokc = tokc;
        if (p->aggregate_target_sym) {
          cpp_default_initializer_receiver.type = *type;
          cpp_default_initializer_receiver.r = p->aggregate_target_sym->r;
          cpp_default_initializer_receiver.sym = p->aggregate_target_sym;
          cpp_default_initializer_receiver.c.i = p->sec
              ? (long long)(c - elfsym(p->aggregate_target_sym)->st_value) : (int)c;
          cpp_default_initializer_receiver_offset = 0;
          if (p->indirect_base) {
            cpp_default_initializer_receiver.c.i = p->indirect_base;
            cpp_default_initializer_receiver_offset = (int)c;
          }
        }
        begin_macro(initializer, 1); next();
        p->list_initialization = p->copy_list_initialization = 0;
        decl_initializer(p, &f->type, c + f->c, DIF_FIRST | DIF_CLEAR);
        p->list_initialization = saved_list;
        p->copy_list_initialization = saved_copy_list;
        cpp_default_initializer_receiver = saved_receiver;
        cpp_default_initializer_receiver_offset = saved_receiver_offset;
        while (macro_stack && macro_stack != parent) end_macro();
        macro_ptr = parent_ptr; tok = saved_tok; tokc = saved_tokc;
      }
    }
    if ((type->t & VT_ARRAY) && !p->sec && !(flags & DIF_SIZE_ONLY)
        && is_cpp_translation_unit()
        && ((t1->t & VT_BTYPE) == VT_STRUCT || (t1->t & VT_ARRAY)))
    {
      /* Omitted class/array elements are value-initialized, including their
         constructors. Zeroing the backing bytes does not start those lifetimes. */
      for (i = indexsym.c; i < n; ++i)
      {
        int temporary_mark = cpp_temp_mark();
        TokenString *empty = tok_str_alloc();
        tok_str_add(empty, '{'); tok_str_add(empty, '}');
        tok_str_add(empty, tok);
        tok_str_add(empty, 0);
        begin_macro(empty, 1); next();
        decl_initializer(p, t1, c + i * size1, DIF_FIRST | DIF_CLEAR);
        cpp_temp_end(temporary_mark);
      }
    }
    if (!no_oblock)
      skip('}');

  }
  else if ((flags & DIF_HAVE_ELEM)
           /* Use i_c_parameter_t, to strip toplevel qualifiers.
              The source type might have VT_CONSTANT set, which is
              of course assignable to non-const elements.  */
           && (is_compatible_unqualified_types(type, &vtop->type)
               || (!p->sec && is_cpp_translation_unit()
                   && (type->t & VT_BTYPE) == VT_STRUCT
                   && type->ref && !type->ref->a.cpp_user_constructor
                   && (vtop->type.t & VT_BTYPE) == VT_STRUCT
                   && try_call_cpp_conversion_operator(type, 0))
               || (!p->sec && is_cpp_translation_unit()
                   && (type->t & VT_BTYPE) == VT_STRUCT && type->ref
                   && type->ref->a.cpp_user_constructor)))
  {
    /* Fall through to the scalar initializer path. */
    if ((flags & DIF_SIZE_ONLY))
    {
      if (flags & DIF_HAVE_ELEM)
        vpop();
      else
        skip_or_save_block(NULL);
    }
    else
    {
      if (!(flags & DIF_HAVE_ELEM))
      {
        /* This should happen only when we haven't parsed
           the init element above for fear of committing a
           string constant to memory too early.  */
        if (!TOK_IS_STRING(tok))
          expect("string constant");
        parse_init_elem(!p->sec ? EXPR_ANY : EXPR_CONST);
      }
      if (p->capture_integral_constexpr
          && initializer_integral_constant_value(vtop,
                                                 &p->integral_constexpr_value))
      {
        if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) == VT_CONST)
          gen_cast(type);
        p->integral_constexpr_valid = 1;
      }
      if (!p->sec && (flags & DIF_CLEAR) // Container Was Already Zero'D
          && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST
          && vtop->c.i == 0
          && btype_size(type->t & VT_BTYPE) // Not For Fp Constants
         )
        vpop();
      else
        init_putv(p, type, c);
    }
  }

  else if ((type->t & VT_BTYPE) == VT_STRUCT
           && type_is_std_initializer_list(type)
           && tok == '{')
  {
    Sym *field;
    TokenString **elements, *initializer = NULL;
    CType *element_type;
    CType backing_type;
    Sym *backing_symbol = NULL;
    int *static_element_addresses = NULL;
    int static_backing = p->extend_backing_array_lifetime == 2;
    int element_storage_kind = static_backing ? VT_LLOCAL | VT_LVAL : VT_LOCAL | VT_LVAL;
    int element_size, element_align, element_addr = 0, element_r2 = 0;
    int i;

    /* List elements are not function arguments: their number is unbounded,
       and a final comma does not introduce an empty element. */
    skip_or_save_block(&initializer);
    len = split_saved_array_initializer(initializer, &elements);
    tok_str_free(initializer);
    if (!(flags & DIF_SIZE_ONLY))
    {
      field = type->ref->next;
      element_type = field ? pointed_type(&field->type) : NULL;
      if (!field || !element_type)
        cprime_error("initializer_list layout missing array field");
      element_size = type_size(element_type, &element_align);
      if (element_size < 0)
        cprime_error("initializer_list element has incomplete type");
      if (len)
      {
        init_params element_init = {0};
        SValue *element_storage;
        if (static_backing)
        {
          char backing_name[64];
          int size = element_size * len;
          int offset = section_add(bss_section, size, element_align);
          backing_type.t = VT_PTR | VT_ARRAY | VT_STATIC;
          backing_type.ref = sym_push2(&global_stack, SYM_FIELD, element_type->t, len);
          backing_type.ref->type.ref = element_type->ref;
          snprintf(backing_name, sizeof(backing_name), "__cpc_list_backing_%u", anon_sym++);
          backing_symbol = external_global_sym(tok_alloc_const(backing_name), &backing_type);
          backing_symbol->type.t = backing_type.t;
          put_extern_sym(backing_symbol, bss_section, offset, size);
          loc = (loc - PTR_SIZE) & -PTR_SIZE;
          element_addr = loc;
          element_r2 = VT_CONST;
          vset(&char_pointer_type, VT_LOCAL | VT_LVAL, element_addr);
          vpushsym(&backing_type, backing_symbol);
          vtop->r |= VT_LVAL;
          mk_pointer(&vtop->type); gaddrof();
          vtop->type = char_pointer_type;
          vstore(); vpop();
          element_init.indirect_base = element_addr;
          static_element_addresses = cprime_malloc(len * sizeof(int));
        }
        else if (p->extend_backing_array_lifetime)
        {
          loc = (loc - element_size * len) & -element_align;
          element_addr = loc;
          element_r2 = VT_CONST;
        }
        else
          element_addr = get_temp_local_var(element_size * len, element_align,
                                            &element_r2);
        element_init.local_offset = (static_backing ? 0 : element_addr) + element_size * len;
        if (!p->extend_backing_array_lifetime)
          cpp_temp_reserve_storage(element_addr, element_size * len, element_r2);
        /* Keep the complete backing allocation live while each element is
           materialized.  Struct element expressions can allocate their own
           temporaries; without this marker the temp allocator reuses and
           overwrites the initializer_list's first slot. */
        vset(element_type, element_storage_kind, element_addr);
        vtop->r2 = element_r2;
        element_storage = vtop;
        for (i = 0; i < len; ++i)
        {
          int element_offset = (static_backing ? 0 : element_addr) + i * element_size;
          int address = element_offset;
          if (static_backing)
          {
            init_push_lvalue(&element_init, element_type, element_offset);
            mk_pointer(&vtop->type); gaddrof();
            vtop->type = char_pointer_type;
            loc = (loc - PTR_SIZE) & -PTR_SIZE;
            address = loc;
            vset(&char_pointer_type, VT_LOCAL | VT_LVAL, address);
            vswap(); vstore(); vpop();
            static_element_addresses[i] = address;
          }
          if ((element_type->t & VT_BTYPE) == VT_STRUCT
              && type_requires_class_construction(element_type))
          {
            /* Each backing-array element is copy-initialized from its
               clause, including ordinary converting constructors. */
            if (saved_initializer_is_direct_constructor(elements[i], element_type))
            {
              element_init.elided_value = emit_class_initializer_expression_at(
                  elements[i], element_type, address, element_storage_kind);
              init_putv(&element_init, element_type, element_offset);
            }
            else
              call_lifecycle_constructor_saved_args_ex(element_type,
                  element_storage_kind, address, element_r2, NULL,
                  resolve_lifecycle_func(element_type, TOK_CONSTRUCTOR1),
                  &elements[i], 1, saved_arg_is_braced(elements[i]) ? 2 : 1);
            vset(element_type, element_storage_kind, address);
            vtop->r2 = VT_CONST;
            cpp_register_class_temporary(vtop);
            if (p->extend_backing_array_lifetime)
              cpp_temp_extend_lifetime(vtop);
            vpop();
            continue;
          }
          TokenString macro = *elements[i];
          int saved_tok = tok;
          CValue saved_tokc = tokc;
          TokenString *saved_macro_stack = macro_stack;
          const int *saved_macro_ptr = macro_ptr;
          begin_macro(&macro, 0);
          next();
          decl_initializer(&element_init, element_type,
                           element_offset, DIF_FIRST);
          while (macro_stack && macro_stack != saved_macro_stack)
            end_macro();
          macro_ptr = saved_macro_ptr;
          tok = saved_tok;
          tokc = saved_tokc;
          if (vtop != element_storage)
            cprime_internal_error(
              "unbalanced initializer_list element initializer");
          if ((element_type->t & VT_BTYPE) == VT_STRUCT)
          {
            vset(element_type, element_storage_kind, address);
            cpp_register_class_temporary(vtop);
            if (p->extend_backing_array_lifetime)
              cpp_temp_extend_lifetime(vtop);
            vpop();
          }
        }
        mk_pointer(&vtop->type);
        gaddrof();
        if (static_backing)
        {
          TokenString *registration = tok_str_alloc();
          TokenString *parent = macro_stack;
          const int *parent_ptr = macro_ptr;
          int saved_tok = tok;
          CValue saved_tokc = tokc;
          tok_str_add(registration, '{');
          append_static_destructor_registration(registration, backing_symbol->v, &backing_type);
          tok_str_add(registration, '}');
          tok_str_add(registration, TOK_EOF);
          begin_macro(registration, 1); next(); block(STMT_COMPOUND);
          while (macro_stack && macro_stack != parent) end_macro();
          macro_ptr = parent_ptr; tok = saved_tok; tokc = saved_tokc;
          for (i = 0; i < len; ++i)
            cpp_temp_commit_static_object(element_storage_kind, static_element_addresses[i]);
          cprime_free(static_element_addresses);
        }
      }
      else
        vpushi(0);
      init_putv(p, &field->type, c + field->c);
      field = field->next;
      if (!field)
        cprime_error("initializer_list layout missing length field");
      vpushi(len);
      init_putv(p, &field->type, c + field->c);
    }
    free_saved_array_elements(elements, len);
  }
  /* A complex object takes its whole value from an expression initializer as
     a scalar conversion; only a braced initializer lists the two parts. */
  else if ((type->t & VT_BTYPE) == VT_STRUCT
           && !(is_complex_type(type) && (flags & DIF_HAVE_ELEM)))
  {
    if (initialize_class_constructor_list(p, type, c, flags)) goto initialized_object;
    no_oblock = 1;
    if ((flags & DIF_FIRST) || tok == '{')
    {
      skip('{');
      no_oblock = 0;
    }
    s = type->ref;
    f = s->next;
    skip_empty_array_fields(&f);
    n = s->c;
    size1 = 1;
    goto do_init_list;

  }
  else if (tok == '{')
  {
    int saved_list_initialization = p->list_initialization;
    p->list_initialization = 1;
    if (flags & DIF_HAVE_ELEM)
      skip(';');
    next();
    if (is_cpp_translation_unit() && tok == '}')
    {
      if (is_reference_type(type))
        cprime_error("reference requires an initializer");
      if (!(flags & DIF_SIZE_ONLY))
      {
        vset(type, VT_CONST, 0);
        init_putv(p, type, c);
      }
    }
    else
      decl_initializer(p, type, c, flags & ~DIF_HAVE_ELEM);
    if (tok == ',') next();
    skip('}');
    p->list_initialization = saved_list_initialization;

  }
  else
  {
    /* If we supported only ISO C we wouldn't have to accept calling
       this on anything than an array if DIF_SIZE_ONLY (and even then
       only on the outermost level, so no recursion would be needed),
       because initializing a flex array member isn't supported.
       But the language supports it, so we need to recurse even into
       subfields of structs and arrays when DIF_SIZE_ONLY is set.  */
    if ((flags & DIF_SIZE_ONLY))
    {
      // Just Skip Expression
      if (flags & DIF_HAVE_ELEM)
        vpop();
      else
        skip_or_save_block(NULL);
    }
    else
    {
      if (!(flags & DIF_HAVE_ELEM))
      {
        /* This should happen only when we haven't parsed
           the init element above for fear of committing a
           string constant to memory too early.  */
        if (!TOK_IS_STRING(tok))
          expect("string constant");
        parse_init_elem(!p->sec ? EXPR_ANY : EXPR_CONST);
      }
      if (p->capture_integral_constexpr
          && initializer_integral_constant_value(vtop,
                                                 &p->integral_constexpr_value))
      {
        if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) == VT_CONST)
          gen_cast(type);
        p->integral_constexpr_valid = 1;
      }
      if (!p->sec && (flags & DIF_CLEAR) // Container Was Already Zero'D
          && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST
          && vtop->c.i == 0
          && btype_size(type->t & VT_BTYPE) // Not For Fp Constants
         )
        vpop();
      else
        init_putv(p, type, c);
    }
  }
initialized_object:
  if (p->track_partial_cleanups && !(flags & DIF_SIZE_ONLY)
      && (type->t & VT_BTYPE) == VT_STRUCT && !nocode_wanted)
  {
    Sym *destructor = resolve_autodtor_func(type);
    if (destructor)
    {
      int pointer_slot;
      /* Replace completed subobject cleanups with the complete object's
         destructor. While later fields initialize, an exception unwinds only
         those subobjects whose initialization has already succeeded. */
      restore_initializer_cleanups(old_cleanup, old_cleanup_depth);
      loc = (loc - PTR_SIZE) & -PTR_SIZE;
      pointer_slot = loc;
      init_push_lvalue(p, type, c);
      mk_pointer(&vtop->type); gaddrof();
      vtop->type = char_pointer_type;
      vset(&char_pointer_type, VT_LOCAL | VT_LVAL, pointer_slot);
      vswap(); vstore(); vpop();
      cpp_eh_register_parameter_copy(type, VT_LLOCAL | VT_LVAL, pointer_slot);
      if (cur_scope->cl.s != old_cleanup)
        cur_scope->cl.s->a.cleanup_exception_only = 1;
    }
  }
}

static void emit_automatic_class_array_initialization(Sym *symbol,
                                                      TokenString *initializer)
{
  CType element = symbol->type;
  TokenString *body = tok_str_alloc(), *path = tok_str_alloc(), *parent;
  const int *parent_ptr;
  Sym *destructor;
  int counter_tok = 0, saved_tok = tok;
  CValue saved_tokc = tokc;
  while ((element.t & VT_ARRAY) && element.ref)
    element = *pointed_type(&element);
  destructor = resolve_autodtor_func(&element);
  if (destructor && !nocode_wanted)
  {
    char name[64];
    CType count_type = {VT_LLONG | VT_UNSIGNED, NULL};
    int pointer_slot, context, alignment;
    loc = (loc - PTR_SIZE) & -PTR_SIZE;
    pointer_slot = loc;
    vset(&char_pointer_type, VT_LOCAL | VT_LVAL, pointer_slot);
    vset(&symbol->type, symbol->r, symbol->c);
    mk_pointer(&vtop->type); gaddrof();
    vtop->type = char_pointer_type;
    vstore(); vpop();
    cpp_eh_register_array_cleanup(pointer_slot, 0,
        type_size(&element, &alignment), destructor, &context);
    snprintf(name, sizeof(name), "__cpc_array_constructed_%u", anon_sym++);
    counter_tok = tok_alloc_const(name);
    sym_push(counter_tok, &count_type, VT_LOCAL | VT_LVAL,
               context + offsetof(CpcEhArray, count));
  }
  tok_str_add(path, symbol->v);
  append_counted_class_initialization(body, &symbol->type, path, initializer,
                                       counter_tok, 1);
  tok_str_add(body, TOK_EOF);
  tok_str_free(path);
  parent = macro_stack; parent_ptr = macro_ptr;
  begin_macro(body, 1);
  next();
  while (tok != TOK_EOF) block(STMT_COMPOUND);
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr;
  tok = saved_tok; tokc = saved_tokc;
}

/* parse an initializer for type 't' if 'has_init' is non zero, and
   allocate space in local or global data space ('r' is either
   VT_LOCAL or VT_CONST). If 'v' is non zero, then an associated
   variable 'v' of scope 'scope' is declared before initializers
   are parsed. If 'v' is zero, then a reference to the new object
   is put in the value stack. If 'has_init' is 2, a special parsing
   is done to handle string constants. */
/* An anonymous union declares one object and exposes each member at that
   object's address in the enclosing scope. */
static void declare_anonymous_aggregate_members(CType *type, AttributeDef *ad,
                                                int scope, int address,
                                                int allocation_size,
                                                int static_storage)
{
  Sym *field;
  for (field = type->ref->next; field; field = field->next) {
    CType member_type = field->type;
    int name = field->v & ~SYM_FIELD;
    Sym *binding, *previous;
    if (name >= SYM_FIRST_ANOM) {
      if ((member_type.t & VT_BTYPE) == VT_STRUCT && member_type.ref)
        declare_anonymous_aggregate_members(&member_type, ad, scope,
                                            address + field->c,
                                            allocation_size, static_storage);
      continue;
    }
    if (field->a.cpp_field_access) cprime_error("anonymous union members must be public");
    if (type_requires_class_construction(&member_type) || type_requires_class_destruction(&member_type))
      cprime_error("anonymous union member requires nontrivial lifetime operations");
    if (scope == VT_CONST) name = make_current_namespace_tok(name);
    previous = sym_find(name);
    if (previous && previous->sym_scope == local_scope)
      cprime_error("anonymous union member redeclares '%s'", get_tok_str(name, NULL));
    member_type.t |= type->t & (VT_CONSTANT | VT_VOLATILE);
    if (static_storage) member_type.t |= VT_STATIC;
    binding = sym_push(name, &member_type,
                       ((member_type.t & VT_ARRAY) ? 0 : VT_LVAL)
                         | (static_storage ? VT_CONST | VT_SYM : VT_LOCAL),
                       static_storage ? 0 : address + field->c);
    binding->a = ad->a;
    if (static_storage)
      put_extern_sym(binding, bss_section, address + field->c,
                     allocation_size);
  }
}

static void declare_anonymous_union(CType *type, AttributeDef *ad, int scope)
{
  MemberFuncOverload *method;
  int size, align, address;
  int static_storage = scope == VT_CONST || (type->t & VT_STATIC);
  for (method = member_func_overloads; method; method = method->next)
    if (method->struct_tok == (type->ref->v & ~SYM_STRUCT))
      cprime_error("anonymous union cannot declare member functions");
  if (scope == VT_CONST && !(type->t & VT_STATIC)
      && !symbol_has_internal_namespace_linkage(make_current_namespace_tok(tok_alloc_const("__anonymous_union"))))
    cprime_error("namespace-scope anonymous union must be static");
  size = type_size(type, &align);
  if (ad->a.aligned && (1 << (ad->a.aligned - 1)) > align) align = 1 << (ad->a.aligned - 1);
  if (static_storage) address = section_add(bss_section, size, align);
  else { loc = (loc - size) & -align; address = loc; }
  declare_anonymous_aggregate_members(type, ad, scope, address, size,
                                      static_storage);
}

static void probe_static_aggregate_initializer(CType *original)
{
  CType type = *original;
  Sym array, record;
  Section section = {0};
  init_params initialization = {0};
  initialization.sec = &section;
  if ((type.t & VT_ARRAY) && type.ref && type.ref->c < 0) {
    array = *type.ref;
    type.ref = &array;
    initialization.flex_array_ref = &array;
  }
  else if ((type.t & VT_BTYPE) == VT_STRUCT && type.ref) {
    Sym *field, *last = type.ref->next, **tail;
    while (last && last->next) last = last->next;
    if (last && (last->type.t & VT_ARRAY) && last->type.ref->c < 0) {
      record = *type.ref;
      type.ref = &record;
      tail = &record.next;
      for (field = original->ref->next; field; field = field->next) {
        Sym *copy = sym_push(field->v | SYM_FIELD, &field->type, field->r, field->c);
        copy->a = field->a;
        *tail = copy;
        tail = &copy->next;
        if (field == last) {
          array = *field->type.ref;
          copy->type.ref = &array;
          initialization.flex_array_ref = &array;
        }
      }
      *tail = NULL;
    }
  }
  decl_initializer(&initialization, &type, 0, DIF_FIRST);
}

static int type_has_mutable_subobject(CType *type)
{
  Sym *field;
  if (type->t & VT_ARRAY)
    return type->ref && type_has_mutable_subobject(&type->ref->type);
  if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref) return 0;
  for (field = type->ref->next; field; field = field->next)
    if (!(field->type.t & (VT_TYPEDEF | VT_STATIC))
        && (field->a.cpp_mutable_field || type_has_mutable_subobject(&field->type))) return 1;
  return 0;
}

/* Every scalar subobject of an evaluation-owned object must carry a recorded
   value before its bytes can describe a constant object.  Collect those values
   into a detached list that survives the evaluation that produced them. */
static int constexpr_collect_object_fields(CType *object_type, CType *type,
                                           Sym *object, int offset,
                                           ConstexprLocalField ***tail)
{
  Sym *field;
  Sym scalar;
  CValue value;
  Sym *relocation;
  ConstexprPointerBounds bounds;
  SValue view;
  ConstexprLocalField *entry;
  int index, align, size;
  if (type->t & VT_ARRAY) {
    if (!type->ref) return 0;
    size = type_size(&type->ref->type, &align);
    if (size <= 0) return 0;
    for (index = 0; index < type->ref->c; ++index)
      if (!constexpr_collect_object_fields(object_type, &type->ref->type,
                                           object, offset + index * size, tail))
        return 0;
    return 1;
  }
  if ((type->t & VT_BTYPE) == VT_STRUCT && type->ref) {
    if (IS_UNION(type->t)) {
      /* A union holds one active member; any recorded alternative is enough. */
      for (field = type->ref->next; field; field = field->next)
        if ((field->v & SYM_FIELD)
            && constexpr_collect_object_fields(object_type, &field->type,
                                               object, offset + field->c, tail))
          return 1;
      return 0;
    }
    for (field = type->ref->next; field; field = field->next)
      if ((field->v & SYM_FIELD) && !(field->type.t & (VT_STATIC | VT_TYPEDEF))
          && !constexpr_collect_object_fields(object_type, &field->type,
                                              object, offset + field->c, tail))
        return 0;
    return 1;
  }
  if ((type->t & VT_VOLATILE)
      || (type->t & VT_BTYPE) == VT_VOID || (type->t & VT_BTYPE) == VT_FUNC)
    return 0;
  memset(&view, 0, sizeof(view));
  view.type = *object_type;
  view.r = VT_LOCAL | VT_LVAL;
  view.c.i = object->c;
  view.sym = object;
  memset(&scalar, 0, sizeof(scalar));
  scalar.type = *type;
  if (!constexpr_aggregate_field_value(&view, &scalar, offset, &value,
                                       &relocation, &bounds))
    return 0;
  /* A value that still names an evaluation-owned object does not outlive the
     evaluation as a constant. */
  if (relocation && is_active_constexpr_temporary(relocation)) return 0;
  entry = cprime_mallocz(sizeof(*entry));
  entry->object = object;
  entry->offset = offset;
  entry->type = *type;
  entry->value = value;
  entry->relocation = relocation;
  entry->pointer_bounds = bounds;
  entry->bit_position = (type->t & VT_BITFIELD) ? BIT_POS(type->t) : 0;
  **tail = entry;
  *tail = &entry->next;
  return 1;
}

static void free_constexpr_object_fields(ConstexprLocalField *fields)
{
  while (fields) {
    ConstexprLocalField *next = fields->next;
    cprime_free(fields);
    fields = next;
  }
}

/* Evaluate a class object's construction and collect the values of its
   subobjects.  1 with *fields set when the construction is a constant. */
static int constant_class_object_evaluate(CType *type, TokenString *initializer,
                                          ConstexprLocalField **fields)
{
  ConstexprTemporaryObject *object, *object_mark = constexpr_temporary_objects;
  TokenString *parent;
  const int *parent_ptr;
  ConstexprLocalField *collected = NULL, **tail = &collected;
  CType object_type = *type;
  int saved_tok, braced, first, done;
  CValue saved_tokc;
  int saved_nocode = nocode_wanted;
  if (!is_cpp_translation_unit()
      || (type->t & VT_BTYPE) != VT_STRUCT || !type->ref) return 0;
  /* A constant object needs no destructor call: its values are emitted as the
     object's data and destruction has no value of its own. */
  if (!constexpr_evaluated_object_type(type)) return 0;
  if (!type_requires_class_construction(type)) return 0;
  /* The object's lifetime has not started yet: qualifying the type would make
     the constructor's own stores look like writes to a const object. */
  object_type.t &= ~(VT_CONSTANT | VT_VOLATILE);
  /* A braced list initializes the object in place; a functional form is an
     ordinary expression that produces the object. */
  first = tok_str_next_non_linenum_index(initializer->str, initializer->len, 0);
  braced = first < initializer->len && initializer->str[first] == '{';
  saved_tok = tok;
  saved_tokc = tokc;
  parent = macro_stack;
  parent_ptr = macro_ptr;
  ++constexpr_temporary_expression_depth;
  nocode_wanted |= CONST_WANTED_BIT;
  begin_macro(initializer, 0);
  next();
  if (braced)
    materialize_braced_temporary_mode(&object_type, 0, NULL);
  else if (tok != TOK_EOF)
    gexpr();
  nocode_wanted = saved_nocode;
  --constexpr_temporary_expression_depth;
  object = NULL;
  if (vtop >= vstack && vtop->sym)
    object = constexpr_temporary_for(vtop->sym);
  vtop = vstack - 1;
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_ptr;
  tok = saved_tok;
  tokc = saved_tokc;
  done = object
         && constexpr_collect_object_fields(&object_type, type,
                                            &object->object, 0, &tail);
  release_constexpr_temporary_objects(object_mark, 1);
  if (!done) {
    free_constexpr_object_fields(collected);
    collected = NULL;
  }
  *fields = collected;
  return done;
}

/* Write collected constant subobject values into a static object's storage. */
static int emit_constant_object_fields(Sym *symbol, ConstexprLocalField *fields)
{
  ObjSym *object_sym;
  Section *section;
  ConstexprLocalField *entry;
  unsigned long base;
  if (!symbol) return 0;
  /* An object with no scalar subobjects needs no bytes of its own. */
  if (!fields) return 1;
  object_sym = elfsym(symbol);
  if (!object_sym || object_sym->st_shndx == 0
      || object_sym->st_shndx >= cprime_state->nb_sections) {
    return 0;
  }
  section = cprime_state->sections[object_sym->st_shndx];
  if (!section || !section->data) return 0;
  base = object_sym->st_value;
  for (entry = fields; entry; entry = entry->next) {
    unsigned char *destination;
    int align, size = type_size(&entry->type, &align);
    if (size <= 0 || (unsigned long)entry->offset + size > section->data_offset)
      return 0;
    destination = (unsigned char *)section->data + base + entry->offset;
    if (entry->relocation) {
      greloca(section, entry->relocation, base + entry->offset, R_DATA_PTR,
              entry->value.i);
      continue;
    }
    if (entry->type.t & VT_BITFIELD) {
      int bits = BIT_SIZE(entry->type.t);
      int position = entry->bit_position;
      unsigned long long mask, current = 0;
      if (bits <= 0 || bits > 64 || position < 0 || size > 8) return 0;
      mask = ((1ULL << bits) - 1) << position;
      switch (size) {
      case 1: current = *(unsigned char *)destination; break;
      case 2: current = read16le(destination); break;
      case 4: current = read32le(destination); break;
      case 8: current = read64le(destination); break;
      default: return 0;
      }
      current = (current & ~mask)
                | (((unsigned long long)entry->value.i << position) & mask);
      switch (size) {
      case 1: *(unsigned char *)destination = (unsigned char)current; break;
      case 2: write16le(destination, current); break;
      case 4: write32le(destination, current); break;
      case 8: write64le(destination, current); break;
      default: return 0;
      }
      continue;
    }
    switch (size) {
    case 1: *(unsigned char *)destination = (unsigned char)entry->value.i; break;
    case 2: write16le(destination, entry->value.i); break;
    case 4: write32le(destination, entry->value.i); break;
    case 8: write64le(destination, entry->value.i); break;
    default: return 0;
    }
  }
  return 1;
}

/* Constant initialization of a class object: run the construction the
   initializer names inside the constant evaluator, then emit the resulting
   subobject values as the object's data. */
static int constant_class_object_initializer(int var_tok, CType *type,
                                             TokenString *initializer)
{
  ConstexprLocalField *fields = NULL;
  Sym *symbol = sym_find(var_tok);
  int valid;
  if (!constant_class_object_evaluate(type, initializer, &fields)) return 0;
  valid = emit_constant_object_fields(symbol, fields);
  free_constexpr_object_fields(fields);
  return valid;
}

static void decl_initializer_alloc(CType *type, AttributeDef *ad, int r,
                                   int has_init, int has_ctor_init,
                                   TokenString *copy_ctor_init, int v, int decl_scope,
                                   int direct_init, int flex_extent)
{
  int size, align, addr;
  TokenString *init_str = NULL;
  TokenString *class_array_initializer = NULL;
  Sym *ctor_func = NULL;
  int has_expr_struct_init, initialize_class_array;
  int initialize_nonlocal_vtables = 0;
  int initialize_declared_constant = 0;

  Section *sec;
  Sym *flexible_array;
  Sym *sym = NULL;
  int saved_nocode_wanted = nocode_wanted;
  int temporary_mark = cpp_temp_mark();
#ifdef CONFIG_CPRIME_BCHECK
  int bcheck = cprime_state->do_bounds_check && !NODATA_WANTED;
#endif
  init_params p = {0};
  p.extend_backing_array_lifetime = v && (r & VT_VALMASK) == VT_LOCAL;
  p.scalar_direct_initialization = direct_init
      && (type->t & VT_BTYPE) == VT_BOOL;
  initialize_class_array = is_cpp_translation_unit() && v
      && (r & VT_VALMASK) == VT_LOCAL && (type->t & VT_ARRAY)
      && !(type->t & VT_VLA)
      && (type_requires_class_construction(type)
          || type_requires_class_destruction(type));
  if (initialize_class_array && has_init)
  {
    skip_or_save_block(&class_array_initializer);
    complete_global_class_array_bound(type, class_array_initializer);
    has_init = 0;
  }
  has_expr_struct_init = has_init
                         && (type->t & VT_BTYPE) == VT_STRUCT;

  if (decl_scope == VT_CONST)
  {
    // see if a global symbol was already defined
    sym = sym_find(v);
    if (sym)
    {
      initialize_declared_constant = !has_init && sym->a.integral_constexpr
                                     && (sym->type.t & VT_EXTERN);
      patch_storage(sym, ad, type);
      if (initialize_declared_constant) sym->r = r | VT_SYM;
      // We Accept Several Definitions Of The Same Global Variable.
      if (!has_init && sym->c && elfsym(sym)->st_shndx != SHN_UNDEF)
        return;
      type = &sym->type;
    }
  }

  // Always allocate static or global variables
  if (v && (r & VT_VALMASK) == VT_CONST)
    nocode_wanted |= DATA_ONLY_WANTED;

  flexible_array = NULL;
  if (type->t & VT_ARRAY) {
    CType *element = &type->ref->type;
    int element_align;
    materialize_incomplete_template_type(element);
    if (type_size(element, &element_align) < 0)
      cprime_error("definition of an array of incomplete type elements");
  }
  size = type_size(type, &align);
  if (size < 0 && (type->t & VT_BTYPE) == VT_STRUCT
      && materialize_incomplete_template_type(type))
    size = type_size(type, &align);

  /* One flexible array can be initialized: the top-level array, the last
     struct member, or the selected member of a top-level union. */

  if (is_cpp_translation_unit()) {
    CType *element = type;
    Sym *destructor;
    cpp_check_abstract_object(type);
    while ((element->t & VT_ARRAY) && element->ref) element = &element->ref->type;
    destructor = resolve_autodtor_func(element);
    if (destructor) {
      cpp_check_deleted_function(&destructor->type);
      check_cpp_function_access(destructor);
    }
  }
  if (size < 0)
  {
    // Error Out Except For Top-Level Incomplete Arrays
    // (Arrays Of Incomplete Types Are Handled In Array Parsing)
    if (!(type->t & VT_ARRAY))
      cprime_error("initialization of incomplete type");
    /* If the base type itself was an array type of unspecified
       size (like in 'typedef int arr[]; arr x = {1};') then
       we will overwrite the unknown size by the real one for
       this decl.  We need to unshare the ref symbol holding
       that size.  */
    if (IS_BT_ARRAY(type->t))
      type->ref = sym_push(SYM_FIELD, &type->ref->type, 0, type->ref->c);
    p.flex_array_ref = type->ref;

  }
  else if (has_init && (type->t & VT_BTYPE) == VT_STRUCT)
  {
    Sym *field = type->ref->next;
    if (IS_UNION(type->t)) {
      for (; field; field = field->next)
        if ((field->type.t & VT_ARRAY) && field->type.ref->c < 0) {
          p.flex_union_fields = type->ref->next;
          size = -1;
          break;
        }
    }
    else if (field)
    {
      while (field->next)
        field = field->next;
      if (field->type.t & VT_ARRAY && field->type.ref->c < 0)
      {
        flexible_array = field;
        p.flex_array_ref = field->type.ref;
        size = -1;
      }
    }
  }
  else if (flex_extent > 0 && size >= 0)
  {
    /* A deferred dynamic initializer still fixes the extent of a trailing
       flexible array member, so the storage has to hold those elements.  The
       member keeps its unknown size for later declarations. */
    Sym *field = flexible_array_member(type);
    if (field)
      size += flex_extent * pointed_size(&field->type);
  }

  if (size < 0)
  {
    // If unknown size, do a dry-run 1st pass
    if (!has_init)
      goto err_size;
    if (has_init == 2)
    {
      // Only Get Strings
      init_str = tok_str_alloc();
      while (TOK_IS_STRING(tok))
      {
        tok_str_add_tok(init_str);
        next();
      }
      tok_str_add(init_str, TOK_EOF);
    }
    else
      skip_or_save_block(&init_str);
    unget_tok(0);

    // Compute Size
    begin_macro(init_str, 1);
    next();
    decl_initializer(&p, type, 0, DIF_FIRST | DIF_SIZE_ONLY);
    // Prepare Second Initializer Parsing
    macro_ptr = init_str->str;
    next();

    // if still unknown size, error
    size = type_size(type, &align);
    if (size < 0)
err_size:
      cprime_error("unknown type size");

    /* If there's a flex member and it was used in the initializer
       adjust size.  */
    if (p.flex_union_fields && p.flex_array_ref) {
      int extent, element_align;
      extent = p.flex_array_ref->c * type_size(&p.flex_array_ref->type, &element_align);
      if (extent > size) size = extent;
    }
    else if (flexible_array && flexible_array->type.ref->c > 0)
      size += flexible_array->type.ref->c
              * pointed_size(&flexible_array->type);

    /* The size-only pass completed a flexible member's extent, and the real
       pass below re-derives it from the same initializer.  A completed count
       of zero would otherwise drop the member from the field list the second
       pass walks, rejecting an initializer such as `{ 1, {} }`. */
    if (flexible_array)
      flexible_array->type.ref->c = -1;
    else if (p.flex_union_fields && p.flex_array_ref)
      p.flex_array_ref->c = -1;
  }

  // take into account specified alignment if bigger
  if (ad->a.aligned)
  {
    int speca = 1 << (ad->a.aligned - 1);
    if (speca > align)
      align = speca;
  }
  else if (ad->a.packed)
    align = 1;

  if (!v && NODATA_WANTED)
    size = 0, align = 1;

  if ((r & VT_VALMASK) == VT_LOCAL)
  {
    int nrvo_result_object = cpp_nrvo_take_result_object(v, type, has_init,
                                                         copy_ctor_init);
    sec = NULL;
#ifdef CONFIG_CPRIME_BCHECK
    if (bcheck && v)
    {
      // Add Padding Between Stack Variables For Bound Checking
      loc -= align;
    }
#endif
    if (nrvo_result_object)
    {
      /* This object is the caller's result object: it gets no frame storage,
         and its constructors must run at the address the caller sees. */
      addr = func_vc;
      p.local_offset = size;
    }
    else
    {
      loc = (loc - size) & -align;
      addr = loc;
      p.local_offset = addr + size;
    }
#ifdef CONFIG_CPRIME_BCHECK
    if (bcheck && v)
    {
      // Add Padding Between Stack Variables For Bound Checking
      loc -= align;
    }
#endif
    if (v)
    {
      // Local Variable
#ifdef CONFIG_CPRIME_ASM
      if (ad->asm_label)
      {
        int reg = asm_parse_regvar(ad->asm_label);
        if (reg >= 0)
          r = (r & ~VT_VALMASK) | reg;
      }
#endif
      sym = sym_push(v, type, nrvo_result_object
                                      ? ((r & ~VT_VALMASK) | VT_LLOCAL) : r,
                     addr);
      if (!NODATA_WANTED)
        cpp_eh_mark_local_initializing(sym);
      {
        CType *destruction_type = type;
        Sym *destructor;
        while ((destruction_type->t & VT_ARRAY) && destruction_type->ref)
          destruction_type = &destruction_type->ref->type;
        destructor = resolve_autodtor_func(destruction_type);
        if (destructor) {
          cpp_check_deleted_function(&destructor->type);
          check_cpp_function_access(destructor);
        }
        /* Array cleanup is emitted by the counted array-initialization path. */
        if (type->t & VT_ARRAY) destructor = NULL;
        if (destructor) {
          Sym *cls = sym_push2(&all_cleanups,
                               SYM_FIELD | ++cur_scope->cl.n, 0, 0);
          cls->cleanup_sym = sym;
          cls->cleanup_func = destructor;
          cls->next = cur_scope->cl.s;
          cur_scope->cl.s = cls;
        }
        else if ((type->t & VT_BTYPE) == VT_STRUCT)
          register_struct_cleanups(type, sym);
      }
      /* Explicit cleanup is additional to C++ destruction. Register it last
         so it runs first on both ordinary scope exit and exception unwinding. */
      if (ad->cleanup_func) {
        Sym *cls = sym_push2(&all_cleanups,
                             SYM_FIELD | ++cur_scope->cl.n, 0, 0);
        cls->cleanup_sym = sym;
        cls->cleanup_func = ad->cleanup_func;
        cls->next = cur_scope->cl.s;
        cur_scope->cl.s = cls;
      }

      sym->a = ad->a;
    }
    else
    {
      // Push Local Reference
      vset(type, r, addr);
    }
  }
  else
  {
    // Allocate Symbol In Corresponding Section
    sec = ad->section;
    if (!sec)
    {
      CType *tp = type;
      while ((tp->t & (VT_BTYPE | VT_ARRAY)) == (VT_PTR | VT_ARRAY))
        tp = &tp->ref->type;
      if (!has_init && type_requires_class_construction(type))
        sec = bss_section;
      else if ((tp->t & VT_CONSTANT) && !type_has_mutable_subobject(type))
        sec = rodata_section;
      else if (has_init)
      {
        sec = data_section;
        /*if (g_debug & 4)
            cprime_warning("rw data: %s", get_tok_str(v, 0));*/
      }
      else if (cprime_state->nocommon)
        sec = bss_section;
    }

    if (sec)
    {
      addr = section_add(sec, size, align);
#ifdef CONFIG_CPRIME_BCHECK
      // add padding if bound check
      if (bcheck)
        section_add(sec, 1, 1);
#endif
    }
    else
    {
      addr = align; // SHN_COMMON is special, symbol value is align
      sec = common_section;
    }

    if (v)
    {
      if (!sym)
      {
        sym = sym_push(v, type, r | VT_SYM, 0);
        patch_storage(sym, ad, NULL);
      }
      // Update Symbol Definition
      put_extern_sym(sym, sec, addr, size);
    }
    else
    {
      // Push Global Reference
      vpush_ref(type, sec, addr, size);
      sym = vtop->sym;
      vtop->r |= r;
    }

#ifdef CONFIG_CPRIME_BCHECK
    /* handles bounds now because the symbol must be defined
       before for the relocation */
    if (bcheck)
    {
      addr_t *bounds_ptr;

      greloca(bounds_section, sym, bounds_section->data_offset, R_DATA_PTR, 0);
      // Then Add Global Bound Info
      bounds_ptr = section_ptr_add(bounds_section, 2 * sizeof(addr_t));
      bounds_ptr[0] = 0; // Relocated
      bounds_ptr[1] = size;
    }
#endif
  }

  if (sym && ad->is_constexpr) sym->cpp_constexpr_object = 1;

  if (type->t & VT_VLA)
  {
    int a;

    if (NODATA_WANTED)
    {
      /* A probe still has to swallow the initializer tokens. */
      if (has_init)
        skip_or_save_block(NULL);
      goto no_alloc;
    }

    // save before-VLA stack pointer if needed
    if (cur_scope->vla.num == 0)
    {
      if (cur_scope->prev && cur_scope->prev->vla.num)
        cur_scope->vla.locorig = cur_scope->prev->vla.loc;
      else
      {
        gen_vla_sp_save(loc -= PTR_SIZE);
        cur_scope->vla.locorig = loc;
      }
    }

    vpush_type_size(type, &a);
    gen_vla_alloc(type, a);
#if defined CPRIME_TARGET_PE && defined CPRIME_TARGET_X86_64
    /* on _WIN64, because of the function args scratch area, the
       result of alloca differs from RSP and is returned in RAX.  */
    gen_vla_result(addr), addr = (loc -= PTR_SIZE);
#endif
    gen_vla_sp_save(addr);
    cur_scope->vla.loc = addr;
    cur_scope->vla.num++;

    if (has_init)
    {
      /* GNU initializes a VLA whose bound is only known at run time by
         zeroing the whole object and then storing the listed elements.
         The size-only replay completes the element count of the synthetic
         array used for the storing pass. */
      CType list_type = *type;
      Sym *list_ref;
      init_params list_init = {0};
      TokenString *list_str = NULL;
      int size_align;

      if (!sym)
        cprime_error("variable length array initializer requires an object");

      list_type.t = (type->t & ~VT_VLA) | VT_ARRAY;
      list_ref = sym_push(SYM_FIELD, pointed_type(type), 0, -1);
      list_type.ref = list_ref;

      skip_or_save_block(&list_str);
      unget_tok(0);
      begin_macro(list_str, 1);
      list_init.flex_array_ref = list_ref;
      next();
      decl_initializer(&list_init, &list_type, 0, DIF_FIRST | DIF_SIZE_ONLY);
      macro_ptr = list_str->str;
      next();
      list_init.list_initialization = tok == '{';

      /* Zero the complete array; its extent is the run-time bound. */
      vpush_helper_func(TOK_memset);
      vset(&char_pointer_type, VT_LOCAL | VT_LVAL, sym->c);
      vpushi(0);
      vpush_type_size(type, &size_align);
      gfunc_call(3);

      /* The extent is a run-time value, so the compile-time bound check of
         init_putv() cannot apply to the stored elements. */
      list_init.indirect_base = sym->c;
      list_init.local_offset = 0x3fffffff;
      decl_initializer(&list_init, &list_type, 0, DIF_FIRST | DIF_CLEAR);
      end_macro();
      next();
    }
  }
  else if (has_init)
  {
    Sym *saved_initialization_object = constant_initialization_object;
    ConstexprTemporaryObject *evaluation_mark = constexpr_temporary_objects;
    int evaluate_temporaries = ad->is_constexpr || (sec && !is_reference_type(type));
    Sym *initial_cleanup = cur_scope ? cur_scope->cl.s : NULL;
    int initial_cleanup_depth = cur_scope ? cur_scope->cl.n : 0;
    p.sec = sec;
    p.constexpr_initializer = ad->is_constexpr;
    if (ad->is_constexpr && !sec && sym
        && ((type->t & VT_BTYPE) == VT_STRUCT || (type->t & VT_ARRAY))) {
      p.constant_object_target = sym;
      p.constant_object_offset = addr;
    }
    p.aggregate_target_sym = sym && (type->t & VT_BTYPE) == VT_STRUCT
                             ? sym : NULL;
    p.track_partial_cleanups = cur_scope && is_cpp_translation_unit() && !sec
                               && (r & VT_VALMASK) == VT_LOCAL
                               && (type->t & VT_BTYPE) == VT_STRUCT && type->ref
                               && !type->ref->a.cpp_user_constructor
                               && !type_is_std_initializer_list(type);
    p.capture_integral_constexpr = is_cpp_translation_unit() && sym
                                   && (type->t & VT_CONSTANT)
                                   && is_integer_btype(type->t & VT_BTYPE)
                                   && !(type->t & (VT_ARRAY | VT_VLA));
    p.object_size_target = sym && (r & VT_VALMASK) == VT_LOCAL
                           && !(type->t & (VT_ARRAY | VT_VLA))
                           && (type->t & VT_BTYPE) != VT_STRUCT
                           ? sym : NULL;
    p.reference_capture_sym = sym && is_reference_type(type)
                              && (r & VT_VALMASK) == VT_LOCAL
                              && !(type->t & VT_STATIC) ? sym : NULL;
    p.pointer_bounds_capture_sym = sec && sym && ad->is_constexpr
        && (type->t & VT_BTYPE) == VT_PTR
        && !(type->t & (VT_ARRAY | VT_VLA)) && !is_reference_type(type) ? sym : NULL;
    p.range_alias_target = sym && (r & VT_VALMASK) == VT_LOCAL
                           && is_integer_btype(type->t & VT_BTYPE)
                           && !(type->t & (VT_ARRAY | VT_VLA | VT_VOLATILE))
                           ? sym : NULL;
    if (sec && !is_reference_type(type)) ++static_initializer_constant_fold;
    p.require_constant_values = ad->is_constexpr && !sec;
    if (p.require_constant_values) ++integral_constant_expression_wanted;
    if (sec || p.constant_object_target) constant_initialization_object = sym;
    if (evaluate_temporaries) ++constexpr_temporary_expression_depth;
    decl_initializer(&p, type, addr, DIF_FIRST);
    if (evaluate_temporaries) {
      --constexpr_temporary_expression_depth;
      release_constexpr_temporary_objects(evaluation_mark, 1);
    }
    constant_initialization_object = saved_initialization_object;
    if (p.require_constant_values) --integral_constant_expression_wanted;
    if (sec && !is_reference_type(type)) --static_initializer_constant_fold;
    if (is_cpp_translation_unit() && sym && sec && sec->data
        && (type->t & VT_CONSTANT) && !(type->t & (VT_ARRAY | VT_VLA | VT_VOLATILE))
        && is_float(type->t) && size > 0 && size <= sizeof(CValue))
    {
      memset(&sym->const_float_value, 0, sizeof(CValue));
      memcpy(&sym->const_float_value, sec->data + addr, size);
      sym->a.static_float_constant = 1;
    }
    if (p.track_partial_cleanups) {
      /* The declaration already owns the complete-object cleanup. Partial
         subobject actions describe only the initializer's exception states. */
      restore_initializer_cleanups(initial_cleanup, initial_cleanup_depth);
    }
    if (p.capture_integral_constexpr && p.integral_constexpr_valid)
    {
      uint64_t raw = (uint64_t)p.integral_constexpr_value;
      if (type->t & VT_UNSIGNED)
        sym->const_value = (long long)raw;
      else if (size == 1)
        sym->const_value = (int8_t)raw;
      else if (size == 2)
        sym->const_value = (int16_t)raw;
      else if (size == 4)
        sym->const_value = (int32_t)raw;
      else
        sym->const_value = (int64_t)raw;
      sym->a.integral_constexpr = 1;
    }
    else if (is_cpp_translation_unit() && sym && decl_scope == VT_CONST
             && (type->t & VT_CONSTANT)
             && is_integer_btype(type->t & VT_BTYPE)
             && !(type->t & (VT_ARRAY | VT_VLA))
             && sec && sec->data && size > 0 && size <= 8)
    {
      uint64_t raw = 0;
      memcpy(&raw, sec->data + addr, size);
      if (type->t & VT_UNSIGNED)
        sym->const_value = (long long)raw;
      else if (size == 1)
        sym->const_value = (int8_t)raw;
      else if (size == 2)
        sym->const_value = (int16_t)raw;
      else if (size == 4)
        sym->const_value = (int32_t)raw;
      else
        sym->const_value = (int64_t)raw;
      sym->a.integral_constexpr = 1;
    }
    // Patch Flexible Array Member Size Back To -1,
    // For Possible Subsequent Similar Declarations
    if (flexible_array)
      flexible_array->type.ref->c = -1;
    if (p.flex_union_fields && p.flex_array_ref)
      p.flex_array_ref->c = -1;
  }

  if (initialize_declared_constant) {
    /* The initializer belongs to the in-class declaration; the definition
       supplies its addressable storage without repeating that initializer. */
    p.sec = sec;
    vpush64(type->t & ~VT_STORAGE, sym->const_value);
    init_putv(&p, type, addr);
  }

  /* Unreachable statements still require constructor argument parsing and
     overload checking; the backend suppresses their generated code. */
  if ((r & VT_VALMASK) == VT_LOCAL && sym && !(type->t & VT_VLA))
  {
    if (has_ctor_init)
      ctor_func = resolve_lifecycle_func(type, TOK_CONSTRUCTOR1);
    else
      ctor_func = resolve_autoctor_func(type);
    if (initialize_class_array)
      emit_automatic_class_array_initialization(sym, class_array_initializer);
    else if (copy_ctor_init)
    {
      TokenString *call_args[1];
      prepare_saved_cpp_lambdas(copy_ctor_init);
      if (saved_initializer_is_direct_constructor(copy_ctor_init, type))
      {
        emit_class_initializer_expression(copy_ctor_init, type, sym->c);
        init_putv(&p, type, sym->c);
      }
      else
      {
        call_args[0] = copy_ctor_init;
        if (!ctor_func)
          ctor_func = resolve_lifecycle_func(type, TOK_CONSTRUCTOR1);
        call_lifecycle_constructor_saved_args(&sym->type, sym->r, sym->c,
                                              VT_CONST, sym, ctor_func,
                                              call_args, 1);
      }
    }
    else if (has_ctor_init)
      call_lifecycle_constructor(&sym->type, sym->r, sym->c, sym, ctor_func);
    else if (!copy_ctor_init)
    {
      if (!has_expr_struct_init)
        call_lifecycle_constructor_members(type, sym->r, sym->c);
      if (ctor_func && !has_expr_struct_init)
        call_lifecycle_constructor_noargs(&sym->type, sym->r, sym->c,
                                          sym, ctor_func);
    }
    /* Constructors run base initialization first, so publish the most-derived
       dispatch tables after construction.  This also covers implicit trivial
       constructors, which otherwise have no generated body in which to set a
       vptr. */
    initialize_virtual_tables_for_object(&sym->type, sym->r, sym->c, sym);
    initialize_virtual_tables_for_member_objects(&sym->type, sym->r, sym->c);
  }
  else if (sym && !NODATA_WANTED && !(type->t & VT_VLA)
           && cur_text_section)
  {
    /* Function-local static and thread-local objects live in a data section,
       but their declaration still executes in function code.  Refreshing
       their most-derived vptrs here is idempotent and covers objects whose
       otherwise-trivial construction needs no emitted constructor call. */
    initialize_nonlocal_vtables = 1;
  }

no_alloc:
  // restore parse state if needed
  if (init_str)
  {
    end_macro();
    next();
  }

  nocode_wanted = saved_nocode_wanted;
  if (initialize_nonlocal_vtables)
    initialize_virtual_tables_for_object(&sym->type, sym->r, sym->c, sym);
  if (sym && (r & VT_VALMASK) == VT_LOCAL && !NODATA_WANTED)
    cpp_eh_finish_local_initialization(sym);
  cpp_temp_end(temporary_mark);
  if (class_array_initializer) tok_str_free(class_array_initializer);
}

// Generate Vla Code Saved In Post_Type()
static void func_vla_arg_code(Sym *arg)
{
  int align;
  TokenString *vla_array_tok = NULL;

  if (arg->type.ref)
    func_vla_arg_code(arg->type.ref);

  if ((arg->type.t & VT_VLA) && arg->type.ref->vla_array_str)
  {
    loc -= type_size(&int_type, &align);
    loc &= -align;
    arg->type.ref->c = loc;

    unget_tok(0);
    vla_array_tok = tok_str_alloc();
    vla_array_tok->str = arg->type.ref->vla_array_str;
    begin_macro(vla_array_tok, 1);
    next();
    gexpr();
    end_macro();
    next();
    vpush_type_size(&arg->type.ref->type, &align);
    gen_op('*');
    vset(&int_type, VT_LOCAL | VT_LVAL, arg->type.ref->c);
    vswap();
    vstore();
    vpop();
  }
}

static void func_vla_arg(Sym *sym)
{
  Sym *arg;

  for (arg = sym->type.ref->next; arg; arg = arg->next)
    if ((arg->type.t & VT_BTYPE) == VT_PTR && (arg->type.ref->type.t & VT_VLA))
      func_vla_arg_code(arg->type.ref);
}

// Set The Local Stack Address For Function Parameter From Gfunc_Prolog()
ST_FUNC Sym *gfunc_set_param(Sym *s, int c, int byref)
{
  s = sym_find(s->v);
  if (!s) // Unnamed Parameters, Not Enabled
    return NULL;
  s->c = c;
  if (byref)
    s->r = VT_LLOCAL | VT_LVAL; // otherwise VT_LOCAL
  return s;
}

/* Publish parameter-scope types, then the actual formal parameter list.
   Deferred template work can allocate unrelated symbols between the function
   prototype and its formals. Allocation order is not parameter ownership. */
static void sym_push_params(Sym *ref)
{
  Sym *s = ref;
  while (s->next)
    s = s->next;
  while (s && s != ref)
  {
    if ((s->sym_scope && sym_is_local_type_binding(s))
        || IS_ENUM_VAL(s->type.t))
      sym_copy(s, &local_stack);
    s = s->prev;
  }
  for (s = ref->next; s; s = s->next)
    if (s->v & ~SYM_FIELD)
      sym_copy(s, &local_stack);
}

/* parse a function defined by symbol 'sym' and generate its code in
   'cur_text_section' */
