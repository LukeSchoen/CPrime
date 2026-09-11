/* Object-size provenance used by __builtin_object_size and
   __builtin_dynamic_object_size.  This is deliberately conservative: when a
   value's extent is not known the builtin answers GCC's unknown sentinel
   instead of guessing. */

static int object_size_info_present(ObjectSizeInfo *info)
{
  return info->whole_known || info->sub_known || info->sub_trailing_root
         || info->sub_union;
}

static int object_size_scalar_type(CType *type)
{
  int bt = type->t & VT_BTYPE;
  return is_integer_btype(bt) || bt == VT_PTR || bt == VT_FLOAT
         || bt == VT_DOUBLE || bt == VT_LDOUBLE || bt == VT_BOOL
         || bt == VT_NULLPTR;
}

static int object_size_integer(SValue *value, long long *result)
{
  if ((value->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST)
  {
    *result = (long long)value->c.i;
    return 1;
  }
  if (value->object_size_int_valid)
  {
    *result = value->object_size_int_value;
    return 1;
  }
  return 0;
}

static long long object_size_symbol_whole(Sym *sym)
{
  int align;
  long long size;

  if (!sym || (sym->type.t & VT_TYPEDEF)
      || (sym->type.t & VT_BTYPE) == VT_FUNC)
    return -1;
  if ((sym->r & (VT_VALMASK | VT_SYM)) == (VT_CONST | VT_SYM))
  {
    ObjSym *esym = elfsym(sym);
    if (esym && esym->st_size)
      return (long long)esym->st_size;
  }
  size = type_size(&sym->type, &align);
  return size >= 0 ? size : -1;
}

static void object_size_load_symbol(SValue *value, Sym *sym)
{
  long long size;
  int is_array = (value->type.t & (VT_ARRAY | VT_VLA)) != 0;
  int is_pointer = (value->type.t & VT_BTYPE) == VT_PTR;

  if (sym && object_size_info_present(&sym->object_size))
  {
    value->object_size = sym->object_size;
    value->object_size_int_value = sym->object_size_int_value;
    value->object_size_int_valid = sym->object_size_int_valid;
    return;
  }
  if ((is_pointer && !is_array) || (value->type.t & VT_TYPEDEF)
      || (value->type.t & VT_BTYPE) == VT_FUNC)
  {
    object_size_reset(&value->object_size);
    value->object_size_int_value = 0;
    value->object_size_int_valid = 0;
    return;
  }

  if (sym)
  {
    value->object_size_int_value = sym->object_size_int_value;
    value->object_size_int_valid = sym->object_size_int_valid;
  }
  else
  {
    value->object_size_int_value = 0;
    value->object_size_int_valid = 0;
  }
  size = object_size_symbol_whole(sym);
  object_size_reset(&value->object_size);
  if (size < 0)
    return;
  value->object_size.whole_known = 1;
  value->object_size.whole_size = size;
  value->object_size.offset_known = 1;
  if (is_array || object_size_scalar_type(&value->type))
  {
    value->object_size.sub_known = 1;
    value->object_size.sub_size = size;
  }
}

static void object_size_store_value(Sym *sym, SValue *value)
{
  long long integer;
  int is_pointer = (value->type.t & VT_BTYPE) == VT_PTR;
  int is_array = (value->type.t & (VT_ARRAY | VT_VLA)) != 0;

  if (!sym || (sym->v & SYM_FIELD) || sym->v >= SYM_FIRST_ANOM)
    return;

  if (is_pointer || is_array)
    sym->object_size = value->object_size;
  else
    object_size_reset(&sym->object_size);

  if (is_integer_btype(value->type.t & VT_BTYPE)
      && object_size_integer(value, &integer))
  {
    sym->object_size_int_value = integer;
    sym->object_size_int_valid = 1;
  }
  else
    sym->object_size_int_valid = 0;
}

static void object_size_set_allocation(SValue *value, long long size)
{
  object_size_reset(&value->object_size);
  value->object_size.whole_known = 1;
  value->object_size.whole_size = size;
  value->object_size.offset_known = 1;
}

static int object_size_call_allocation_size(Sym *call, SValue *args,
                                            int nb_args, long long *size)
{
  const char *name;
  long long first, second;

  if (!call || nb_args < 1)
    return 0;
  name = get_tok_str(call->v, NULL);
  if (!name)
    return 0;

  if (!strcmp(name, "malloc") || !strcmp(name, "__builtin_malloc")
      || !strcmp(name, "alloca") || !strcmp(name, "__builtin_alloca"))
  {
    if (!object_size_integer(&args[0], &first) || first < 0)
      return 0;
    *size = first;
    return 1;
  }

  if (!strcmp(name, "calloc") || !strcmp(name, "__builtin_calloc"))
  {
    if (nb_args < 2 || !object_size_integer(&args[0], &first)
        || !object_size_integer(&args[1], &second)
        || first < 0 || second < 0)
      return 0;
    *size = first * second;
    return 1;
  }

  return 0;
}

static void object_size_apply_member(ObjectSizeInfo *out, CType *container,
                                     Sym *field, int cumofs,
                                     ObjectSizeInfo *base)
{
  int align;
  long long field_size;
  int is_array = (field->type.t & (VT_ARRAY | VT_VLA)) != 0;
  int is_flexible = is_array && field->type.ref && field->type.ref->c < 0;
  int is_union = IS_UNION(container->t);
  int is_last = field->next == NULL;
  int root_direct = base->depth == 0;

  *out = *base;
  out->depth = base->depth < 255 ? base->depth + 1 : base->depth;
  if (out->offset_known)
    out->offset += cumofs;
  out->sub_trailing_root = 0;

  field_size = type_size(&field->type, &align);
  if (field_size < 0)
    field_size = -1;

  if (base->sub_union)
  {
    out->sub_known = 0;
    out->sub_union = 1;
  }
  else if (is_union && !is_array)
  {
    out->sub_known = 0;
    out->sub_union = 1;
  }
  else if (is_array)
  {
    out->sub_known = !is_flexible && field_size >= 0;
    out->sub_size = field_size;
    out->sub_trailing_root = root_direct && is_last;
    if (out->sub_trailing_root && !out->whole_known)
      out->sub_known = 0;
  }
  else if (object_size_scalar_type(&field->type))
  {
    out->sub_known = field_size >= 0;
    out->sub_size = field_size;
  }
  else
  {
    out->sub_known = 0;
  }
}

static void object_size_apply_index(ObjectSizeInfo *out, int index_known,
                                    long long index, long long element_size,
                                    int element_struct,
                                    ObjectSizeInfo *base)
{
  *out = *base;
  out->sub_trailing_root = 0;

  if (index_known && out->offset_known)
    out->offset += index * element_size;
  else
    out->offset_known = 0;

  if (base->sub_union)
  {
    out->sub_union = 1;
    out->sub_known = 0;
    return;
  }
  if (element_struct)
  {
    out->sub_known = 0;
    return;
  }
  if (base->sub_known && base->sub_size >= 0 && index_known)
  {
    long long remaining = base->sub_size - index * element_size;
    out->sub_known = 1;
    out->sub_size = remaining > 0 ? remaining : 0;
  }
  else
  {
    out->sub_known = 0;
  }
}

static long long object_size_result(ObjectSizeInfo *info, int mode)
{
  long long unknown = (mode & 2) ? 0 : -1;
  long long value;

  if (mode & 1)
  {
    if (info->sub_union)
      return unknown;
    if (info->sub_trailing_root)
    {
      if (!info->whole_known || !info->offset_known)
        return unknown;
      value = info->whole_size - info->offset;
    }
    else if (info->sub_known)
    {
      value = info->sub_size;
      if (info->whole_known && info->offset_known
          && value > info->whole_size - info->offset)
        value = info->whole_size - info->offset;
    }
    else if (info->whole_known && info->offset_known)
      value = info->whole_size - info->offset;
    else
      return unknown;
  }
  else
  {
    if (!info->whole_known || !info->offset_known)
      return unknown;
    value = info->whole_size - info->offset;
  }

  return value > 0 ? value : 0;
}
