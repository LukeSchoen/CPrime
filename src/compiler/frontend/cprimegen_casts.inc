/* Named casts have separate admissibility rules even where their final
   machine representation is shared with a C-style cast. */
static void cpp_validate_pointer_integer_cast(CType *target)
{
  int from = vtop->type.t & VT_BTYPE, to = target->t & VT_BTYPE, alignment;
  if (is_cpp_translation_unit() && (from == VT_PTR || from == VT_FUNC)
      && is_integer_btype(to) && to != VT_BOOL
      && type_size(target, &alignment) < PTR_SIZE)
    cast_error(&vtop->type, target);
}

static int cpp_cast_similar_types(CType *left, CType *right)
{
  CppMemberPointerType *lm = find_cpp_member_pointer_type(left);
  CppMemberPointerType *rm = find_cpp_member_pointer_type(right);
  if (lm || rm)
    return lm && rm && lm->owner_tok == rm->owner_tok
        && cpp_cast_similar_types(&lm->member_type, &rm->member_type);
  if ((left->t & VT_BTYPE) == VT_PTR && (right->t & VT_BTYPE) == VT_PTR)
    return cpp_cast_similar_types(pointed_type(left), pointed_type(right));
  return is_compatible_unqualified_types(left, right);
}

static int cpp_cast_discards_qualifiers(CType *target, CType *source)
{
  for (;;) {
    if ((source->t & ~target->t) & (VT_CONSTANT | VT_VOLATILE)) return 1;
    if ((target->t & VT_BTYPE) != VT_PTR || (source->t & VT_BTYPE) != VT_PTR)
      return 0;
    target = pointed_type(target);
    source = pointed_type(source);
  }
}

static void cpp_validate_named_cast(CType *target, int cast_name)
{
  CType source = vtop->type, destination = *target;
  int reference = is_reference_type(target), from, to, alignment;
  const char *name = get_tok_str(cast_name, NULL);
  if (!strcmp(name, "dynamic_cast")) return;
  if (is_reference_type(&source)) source = *pointed_type(&source);
  if (reference) destination = *pointed_type(&destination);
  from = source.t & VT_BTYPE;
  to = destination.t & VT_BTYPE;
  if (!strcmp(name, "const_cast")) {
    if ((!reference && to != VT_PTR && !find_cpp_member_pointer_type(&destination))
        || !cpp_cast_similar_types(&destination, &source)
        || (to == VT_PTR && (pointed_type(&destination)->t & VT_BTYPE) == VT_FUNC))
      cast_error(&source, target);
    return;
  }
  if (!strcmp(name, "reinterpret_cast")) {
    CppMemberPointerType *member = find_cpp_member_pointer_type(&destination);
    CppMemberPointerType *source_member = find_cpp_member_pointer_type(&source);
    int valid = reference || (to == VT_PTR && (from == VT_PTR || from == VT_FUNC
                    || is_integer_btype(from)))
        || (is_integer_btype(to) && (from == VT_PTR || from == VT_FUNC)
            && type_size(&destination, &alignment) >= PTR_SIZE)
        || (is_integer_btype(to) && is_integer_btype(from)
            && is_compatible_unqualified_types(&destination, &source))
        || (member && source_member
            && ((member->member_type.t & VT_BTYPE) == VT_FUNC)
                == ((source_member->member_type.t & VT_BTYPE) == VT_FUNC));
    if (!valid || ((reference || (to == VT_PTR && from == VT_PTR))
                  && cpp_cast_discards_qualifiers(&destination, &source)))
      cast_error(&source, target);
    return;
  }
  if (strcmp(name, "static_cast") || reference) return;
  if (from == VT_PTR && is_integer_btype(to) && to != VT_BOOL)
    cast_error(&source, target);
  if (to == VT_PTR && is_integer_btype(from) && !is_null_pointer(vtop))
    cast_error(&source, target);
  if (to == VT_PTR && from == VT_PTR) {
    CType *pointee = pointed_type(&destination), *original = pointed_type(&source);
    int left, right, offset;
    materialize_incomplete_template_type(pointee);
    materialize_incomplete_template_type(original);
    left = get_struct_type_name_tok(pointee);
    right = get_struct_type_name_tok(original);
    if (cpp_cast_discards_qualifiers(pointee, original)
        || (!cpp_cast_similar_types(pointee, original)
            && (pointee->t & VT_BTYPE) != VT_VOID
            && (original->t & VT_BTYPE) != VT_VOID
            && !(left && right && (class_has_base(left, right) || class_has_base(right, left)))))
      cast_error(&source, target);
    if (left && right && left != right
        && ((class_has_base(right, left) && !cpp_rtti_public_base_offset(right, left, &offset)
             && !(active_member_class_tok == right
                  && class_static_downcast_offset(right, left, &offset) == 1))
            || (class_has_base(left, right)
                && class_static_downcast_offset(left, right, &offset) != 1)))
      cast_error(&source, target);
  }
}

static int cpp_static_pointer_downcast(CType *target)
{
  int source_class, target_class, offset, null_jump = 0;
  if ((target->t & VT_BTYPE) != VT_PTR || (vtop->type.t & VT_BTYPE) != VT_PTR)
    return 0;
  source_class = get_struct_type_name_tok(pointed_type(&vtop->type));
  target_class = get_struct_type_name_tok(pointed_type(target));
  if (!source_class || !target_class || source_class == target_class
      || !class_has_base(target_class, source_class)) return 0;
  if (class_static_downcast_offset(target_class, source_class, &offset) != 1)
    cast_error(&vtop->type, target);
  if (offset && !is_null_pointer(vtop)) {
    vtop->type = char_pointer_type;
    gv(RC_INT); vdup(); null_jump = gvtst(1, 0);
    vpushi(offset); gen_op('-');
    gsym(null_jump);
  }
  vtop->type = *target;
  return 1;
}
