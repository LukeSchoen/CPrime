#include "../../../include/runtime/cprime_rtti.h"

typedef struct CppTypeInfoRecord {
  int name_tok;
  Sym *symbol;
  struct CppTypeInfoRecord *next;
} CppTypeInfoRecord;
static CppTypeInfoRecord *cpp_type_info_records;

static Sym *cpp_type_info_for_type(CType *type);

static int cpp_class_is_polymorphic(int class_tok)
{
  VirtualMethodInfo *method;
  for (method = virtual_method_infos; method; method = method->next)
    if (method->class_tok == class_tok || class_has_base(class_tok, method->class_tok))
      return 1;
  return 0;
}

static void free_cpp_type_info_records(void)
{
  while (cpp_type_info_records) {
    CppTypeInfoRecord *record = cpp_type_info_records;
    cpp_type_info_records = record->next;
    cprime_free(record);
  }
}

static int cpp_type_info_class(CType *type)
{
  int parts[2] = { tok_alloc_const("std"), tok_alloc_const("type_info") };
  return make_class_type_from_tok(type, make_namespace_tok_from_parts(parts, 2));
}

/* Itanium describes a pointer type_info through __pbase_type_info: the
   top-level qualifiers of the pointed-to type live in __flags and __pointee
   names the pointee's own type_info. When the translation unit declares that
   ABI class, the emitted descriptor takes its shape so the pointer
   information can be read through it. */
static int cpp_type_info_abi_pointer_class(CType *type, CType *info_type)
{
  CType abi_type;
  int parts[2], offset;
  if ((type->t & VT_BTYPE) != VT_PTR)
    return 0;
  parts[0] = tok_alloc_const("__cxxabiv1");
  parts[1] = tok_alloc_const("__pointer_type_info");
  if (!make_class_type_from_tok(&abi_type, make_namespace_tok_from_parts(parts, 2)))
    return 0;
  if (type_size(&abi_type, &offset) < 0)
    return 0;
  /* The ABI class has to inherit the runtime's name field; otherwise the
     emitted object would not be readable through the declared layout. */
  if (!find_field_try(&abi_type, tok_alloc_const("_M_name"), &offset))
    return 0;
  *info_type = abi_type;
  return 1;
}

static void cpp_type_info_abi_pointer_fields(CType *type, CType *info_type, Sym *owner)
{
  CType *pointee;
  CType element;
  Sym *field;
  int field_offset;
  unsigned flags = 0;
  pointee = pointed_type(type);
  /* Array qualification belongs to its element type, as in the Itanium
     encoding of the pointee. */
  element = *pointee;
  while (element.t & VT_ARRAY)
    element = *pointed_type(&element);
  if (element.t & VT_CONSTANT) flags |= 0x1;
  if (element.t & VT_VOLATILE) flags |= 0x2;
  field = find_field_try(info_type, tok_alloc_const("__flags"), &field_offset);
  if (field && flags)
    memcpy(data_section->data + elfsym(owner)->st_value + field_offset,
           &flags, sizeof flags);
  field = find_field_try(info_type, tok_alloc_const("__pointee"), &field_offset);
  if (field)
    cpp_eh_data_pointer(owner, field_offset, cpp_type_info_for_type(pointee));
}

static void cpp_type_info_append_length_name(CString *name, int tok)
{
  char prefix[32];
  const char *spelling = get_tok_str(tok, NULL);
  int index;
  if (!spelling) spelling = "";
  index = tok - TOK_IDENT;
  if (index >= 0 && index < class_source_name_capacity
      && class_source_name_toks[index])
    spelling = get_tok_str(class_source_name_toks[index], NULL);
  snprintf(prefix, sizeof(prefix), "%u", (unsigned)strlen(spelling));
  cstr_cat(name, prefix, -1);
  cstr_cat(name, spelling, -1);
}

static int cpp_type_info_append_function_parameter(CString *name, CType *type)
{
  int bt = type->t & VT_BTYPE;
  if (type->t & VT_CONSTANT) cstr_cat(name, "K", -1);
  if (type->t & VT_VOLATILE) cstr_cat(name, "V", -1);
  if (type->t & VT_REFERENCE) {
    cstr_cat(name, "R", -1);
    return cpp_type_info_append_function_parameter(name, pointed_type(type));
  }
  if (type->t & VT_RVALUE_REFERENCE) {
    cstr_cat(name, "O", -1);
    return cpp_type_info_append_function_parameter(name, pointed_type(type));
  }
  if (bt == VT_PTR) {
    cstr_cat(name, "P", -1);
    return cpp_type_info_append_function_parameter(name, pointed_type(type));
  }
  if (bt == VT_STRUCT) {
    int class_tok = get_struct_type_name_tok(type);
    if (!class_tok) return 0;
    cpp_type_info_append_length_name(name, class_tok);
    return 1;
  }
  switch (bt) {
  case VT_VOID: cstr_cat(name, "v", -1); return 1;
  case VT_BOOL: cstr_cat(name, "b", -1); return 1;
  case VT_BYTE:
    cstr_cat(name, (type->t & VT_UNSIGNED) ? "h" : "c", -1);
    return 1;
  case VT_SHORT:
    cstr_cat(name, (type->t & VT_UNSIGNED) ? "t" : "s", -1);
    return 1;
  case VT_INT:
    if (type->t & VT_LONG) {
      cstr_cat(name, (type->t & VT_UNSIGNED) ? "m" : "l", -1);
      return 1;
    }
    cstr_cat(name, (type->t & VT_UNSIGNED) ? "j" : "i", -1);
    return 1;
  case VT_LLONG:
    cstr_cat(name, (type->t & VT_UNSIGNED) ? "y" : "x", -1);
    return 1;
  case VT_FLOAT: cstr_cat(name, "f", -1); return 1;
  case VT_DOUBLE: cstr_cat(name, "d", -1); return 1;
  case VT_LDOUBLE: cstr_cat(name, "e", -1); return 1;
  default: return 0;
  }
}

static int cpp_type_info_append_function_name(CString *name, int function_tok)
{
  Sym *function = global_symbol_find(function_tok);
  Sym *parameter;
  char length[32];
  const char *spelling;
  if (!function) function = sym_find(function_tok);
  spelling = cpp_source_function_name();
  if (!spelling || !*spelling) spelling = get_tok_str(function_tok, NULL);
  if (!spelling) return 0;
  snprintf(length, sizeof(length), "%u", (unsigned)strlen(spelling));
  cstr_cat(name, length, -1);
  cstr_cat(name, spelling, -1);
  parameter = function && function->type.ref ? function->type.ref->next : NULL;
  if (!parameter) {
    cstr_cat(name, "v", -1);
    return 1;
  }
  for (; parameter; parameter = parameter->next)
    if (!cpp_type_info_append_function_parameter(name, &parameter->type))
      return 0;
  return 1;
}

static int cpp_type_info_name_itanium(CString *name, CType *type, int strip_cv)
{
  CType plain;
  int bt;
  int function_owner;
  int class_tok;
  char bound[48];
  if (is_reference_type(type))
    type = pointed_type(type);
  plain = *type;
  if (strip_cv)
    plain.t &= ~(VT_CONSTANT | VT_VOLATILE);
  type = &plain;
  bt = type->t & VT_BTYPE;
  if (type->t & VT_CONSTANT) cstr_cat(name, "K", -1);
  if (type->t & VT_VOLATILE) cstr_cat(name, "V", -1);
  if (type->t & VT_ARRAY) {
    cstr_cat(name, "A", -1);
    if (type->ref && type->ref->c > 0) {
      snprintf(bound, sizeof(bound), "%lld", (long long)type->ref->c);
      cstr_cat(name, bound, -1);
    }
    cstr_cat(name, "_", -1);
    return cpp_type_info_name_itanium(name, pointed_type(type), strip_cv);
  }
  if (bt == VT_PTR) {
    cstr_cat(name, "P", -1);
    return cpp_type_info_name_itanium(name, pointed_type(type), 0);
  }
  if (bt == VT_FUNC)
    return 0;
  if (bt == VT_STRUCT) {
    class_tok = get_struct_type_name_tok(type);
    if (!class_tok) class_tok = cpp_unnamed_tag_typedef_tok(type);
    if (!class_tok) return 0;
    function_owner = local_type_function_owner(class_tok);
    if (function_owner && function_owner != class_tok) {
      cstr_cat(name, "Z", -1);
      if (!cpp_type_info_append_function_name(name, function_owner))
        return 0;
      cstr_cat(name, "E", -1);
    }
    cpp_type_info_append_length_name(name, class_tok);
    return 1;
  }
  switch (bt) {
  case VT_VOID: cstr_cat(name, "v", -1); return 1;
  case VT_BOOL: cstr_cat(name, "b", -1); return 1;
  case VT_BYTE:
    cstr_cat(name, (type->t & VT_UNSIGNED) ? "h" : "c", -1);
    return 1;
  case VT_SHORT:
    cstr_cat(name, (type->t & VT_UNSIGNED) ? "t" : "s", -1);
    return 1;
  case VT_INT:
    if (type->t & VT_LONG) {
      cstr_cat(name, (type->t & VT_UNSIGNED) ? "m" : "l", -1);
      return 1;
    }
    cstr_cat(name, (type->t & VT_UNSIGNED) ? "j" : "i", -1);
    return 1;
  case VT_LLONG:
    cstr_cat(name, (type->t & VT_UNSIGNED) ? "y" : "x", -1);
    return 1;
  case VT_FLOAT: cstr_cat(name, "f", -1); return 1;
  case VT_DOUBLE: cstr_cat(name, "d", -1); return 1;
  case VT_LDOUBLE: cstr_cat(name, "e", -1); return 1;
  default: return 0;
  }
}

static void cpp_type_info_name_ex(CString *name, CType *original, int strip_cv)
{
  CType type = *original;
  if (is_reference_type(&type)) type = *pointed_type(&type);
  type.t &= ~(VT_STORAGE | VT_RVALUE_REFERENCE);
  if (strip_cv) type.t &= ~(VT_CONSTANT | VT_VOLATILE);
  if (type.t & VT_BITFIELD) type.t &= ~VT_STRUCT_MASK;
  if (!cpp_type_info_name_itanium(name, &type, strip_cv))
    cprime_error("unsupported typeid type");
}

static Sym *cpp_type_info_for_type(CType *type)
{
  CType info_type;
  CType base_info_type;
  CString name;
  CppTypeInfoRecord *record;
  VirtualTableInfo *table;
  Sym *name_symbol, *field;
  int name_tok, size, alignment, field_offset;
  int abi_pointer = 0;
  if (!cpp_type_info_class(&info_type)) return NULL;
  base_info_type = info_type;
  cstr_new(&name);
  cpp_type_info_name_ex(&name, type, 1);
  cstr_ccat(&name, 0);
  name_tok = tok_alloc_const(name.data);
  for (record = cpp_type_info_records; record; record = record->next)
    if (record->name_tok == name_tok) { cstr_free(&name); return record->symbol; }
  abi_pointer = cpp_type_info_abi_pointer_class(type, &info_type);
  size = type_size(&info_type, &alignment);
  if (size < 0) cprime_error("typeid requires a complete std::type_info");
  record = cprime_mallocz(sizeof(*record));
  record->name_tok = name_tok;
  record->symbol = cpp_eh_data_symbol(NULL, size, alignment);
  record->next = cpp_type_info_records;
  cpp_type_info_records = record;
  name_symbol = cpp_eh_data_symbol(name.data, name.size, 1);
  cstr_free(&name);
  field_offset = 0;
  field = find_field_try(&info_type, tok_alloc_const("_M_name"), &field_offset);
  if (field)
    cpp_eh_data_pointer(record->symbol, field_offset, name_symbol);
  for (table = virtual_table_infos; table; table = table->next)
    /* The runtime owns std::type_info's representation, so the object keeps
       the runtime table even when the ABI spelling widens the layout. */
    if (table->class_tok == get_struct_type_name_tok(&base_info_type)) {
      int offset = table->vptr_offset;
      table->referenced = 1;
      cpp_eh_data_pointer(record->symbol, offset, table->sym);
    }
  if (abi_pointer)
    cpp_type_info_abi_pointer_fields(type, &info_type, record->symbol);
  return record->symbol;
}

static void cpp_rtti_collect_from(int complete, int class_tok, int offset, int parent, int access,
                             CppRttiNode **nodes, int *count)
{
  ClassBaseInfo *base;
  int index = (*count)++;
  *nodes = cprime_realloc(*nodes, *count * sizeof(**nodes));
  (*nodes)[index].class_tok = class_tok;
  (*nodes)[index].offset = offset;
  (*nodes)[index].parent = parent;
  (*nodes)[index].public_base = access == 0;
  for (base = class_base_candidates(class_tok); base; base = base->bucket_next)
    if (base->class_tok == class_tok && base->field) {
      int base_offset = offset + base->field->c;
      /* Repeated inheritance paths describe one virtual base address in
         the complete object, including its nonvirtual base subobjects. */
      if (base->is_virtual)
        class_subobject_offset(complete, base->base_tok, &base_offset);
      cpp_rtti_collect_from(complete, base->base_tok, base_offset, index,
                            base->access, nodes, count);
    }
}

static void cpp_rtti_collect(int class_tok, int offset, int parent, int access,
                              CppRttiNode **nodes, int *count)
{
  cpp_rtti_collect_from(class_tok, class_tok, offset, parent, access, nodes, count);
}

static Sym *cpp_rtti_type_name(CType *original)
{
  CString name;
  CType type = *original;
  Sym *symbol;
  type.t &= ~(VT_STORAGE | VT_CONSTANT | VT_VOLATILE | VT_RVALUE_REFERENCE);
  cstr_new(&name);
  append_type_mangle_string(&name, &type);
  cstr_ccat(&name, 0);
  symbol = cpp_eh_data_symbol(name.data, name.size, 1);
  cstr_free(&name);
  return symbol;
}

static Sym *cpp_rtti_table_descriptor(int class_tok, int vptr_offset)
{
  CppRttiNode *nodes = NULL;
  CpcRttiTable table;
  Sym *entries, *descriptor;
  int count = 0, i;
  cpp_rtti_collect(class_tok, 0, -1, 0, &nodes, &count);
  entries = cpp_eh_data_symbol(NULL, count * sizeof(CpcRttiNode), PTR_SIZE);
  for (i = 0; i < count; ++i) {
    CType type;
    CpcRttiNode node;
    Sym *name;
    if (!make_class_type_from_tok(&type, nodes[i].class_tok))
      cprime_error("runtime type metadata requires a complete class");
    name = cpp_rtti_type_name(&type);
    memset(&node, 0, sizeof node);
    node.offset = nodes[i].offset;
    node.parent = nodes[i].parent;
    node.public_base = nodes[i].public_base;
    memcpy(data_section->data + elfsym(entries)->st_value + i * sizeof node,
           &node, sizeof node);
    cpp_eh_data_pointer(entries, i * sizeof node + offsetof(CpcRttiNode, name), name);
  }
  memset(&table, 0, sizeof table);
  table.offset_to_top = -vptr_offset;
  table.count = count;
  {
    CType type;
    int alignment;
    if (make_class_type_from_tok(&type, class_tok))
      table.object_size = type_size(&type, &alignment);
  }
  descriptor = cpp_eh_data_symbol(&table, sizeof table, PTR_SIZE);
  cpp_eh_data_pointer(descriptor, offsetof(CpcRttiTable, nodes), entries);
  {
    CType type;
    if (make_class_type_from_tok(&type, class_tok))
      cpp_eh_data_pointer(descriptor, offsetof(CpcRttiTable, type_info), cpp_type_info_for_type(&type));
  }
  cprime_free(nodes);
  return descriptor;
}

static int cpp_rtti_public_base_offset(int source, int target, int *offset)
{
  CppRttiNode *nodes = NULL;
  int count = 0, i, found = 0, accessible = 0;
  cpp_rtti_collect(source, 0, -1, 0, &nodes, &count);
  for (i = 0; i < count; ++i) if (nodes[i].class_tok == target) {
    int parent = i, is_public = 1;
    if (found && *offset != nodes[i].offset) {
      cprime_free(nodes);
      return 0;
    }
    *offset = nodes[i].offset;
    found = 1;
    while (parent > 0) {
      is_public &= nodes[parent].public_base;
      parent = nodes[parent].parent;
    }
    accessible |= is_public;
  }
  cprime_free(nodes);
  return found && accessible;
}

static void cpp_virtual_base_cast(CType *target)
{
  CType source_type = *pointed_type(&vtop->type);
  int source = get_struct_type_name_tok(&source_type);
  int root = class_primary_virtual_root(source);
  int offset = virtual_vptr_offset(source, root);
  SValue operand;
  Sym *source_name, *target_name;
  if (NODATA_WANTED || is_null_pointer(vtop)) { vtop->type = *target; return; }
  if (offset < 0) cprime_error("virtual base conversion requires runtime base metadata");
  source_name = cpp_rtti_type_name(&source_type);
  target_name = cpp_rtti_type_name(pointed_type(target));
  operand = *vtop--;
  cpp_eh_push_helper("__cpc_virtual_base_cast");
  vpushv(&operand); vpushll(offset);
  vpushsym(&char_pointer_type, source_name);
  vpushsym(&char_pointer_type, target_name);
  gfunc_call(4);
  vset(target, REG_IRET, 0);
}

static void cpp_typeid_expression(void)
{
  TokenString *operand = tok_str_alloc(), *saved_macro;
  const int *saved_ptr;
  CType type, info_type;
  CValue saved_value;
  VirtualTableInfo *table;
  int depth = 0, type_operand, expression_flags = 0, saved_token, offset = -1, alignment;
  if (!is_cpp_translation_unit()) cprime_error("typeid requires C++");
  if (!cpp_type_info_class(&info_type)) cprime_error("include <typeinfo> before using typeid");
  next(); skip('(');
  while (tok != TOK_EOF && (tok != ')' || depth)) {
    if (tok == '(') ++depth;
    else if (tok == ')') --depth;
    tok_str_add_tok(operand); next();
  }
  skip(')');
  tok_str_add(operand, TOK_EOF);
  saved_token = tok; saved_value = tokc;
  saved_macro = macro_stack; saved_ptr = macro_ptr;
  type_operand = template_probe_type(operand, 6, current_namespace_scope_tok(), &type);
  if (!type_operand) {
    begin_macro(operand, 0); next();
    ++nocode_wanted; ++cpp_unevaluated_expression_depth;
    gexpr();
    type = vtop->type;
    expression_flags = vtop->r;
    vpop();
    --cpp_unevaluated_expression_depth; --nocode_wanted;
    if (tok != TOK_EOF) cprime_error("invalid typeid operand");
    while (macro_stack && macro_stack != saved_macro) end_macro();
    macro_ptr = saved_ptr; tok = saved_token; tokc = saved_value;
  }
  if (is_reference_type(&type)) type = *pointed_type(&type);
  if ((type.t & VT_BTYPE) == VT_STRUCT && type_size(&type, &alignment) < 0)
    cprime_error("typeid requires a complete class type");
  info_type.t |= VT_CONSTANT;
  if (!type_operand && (expression_flags & VT_LVAL)
      && !(expression_flags & VT_CXX_PRVALUE)
      && (type.t & VT_BTYPE) == VT_STRUCT
      && cpp_class_is_polymorphic(get_struct_type_name_tok(&type)))
    cpp_noexcept_note_throw();
  if (NOEVAL_WANTED) {
    tok_str_free(operand);
    vset(&info_type, VT_CONST | VT_LVAL, 0);
    return;
  }
  if (!type_operand && (expression_flags & VT_LVAL)
      && !(expression_flags & VT_CXX_PRVALUE)
      && (type.t & VT_BTYPE) == VT_STRUCT
      && cpp_class_is_polymorphic(get_struct_type_name_tok(&type)))
    for (table = virtual_table_infos; table; table = table->next)
      if (table->class_tok == get_struct_type_name_tok(&type)) {
        offset = virtual_vptr_offset(table->class_tok, table->root_tok);
        if (offset >= 0) break;
      }
  if (offset >= 0) {
    Sym *helper = global_symbol_find(tok_alloc_const("__cpc_typeid_dynamic"));
    if (!helper) cprime_error("typeid runtime helper is not declared");
    vpushsym(&helper->type, helper);
    begin_macro(operand, 0); next(); gexpr();
    maybe_indir_reference();
    mk_pointer(&vtop->type); gaddrof();
    gfunc_param_typed(helper->type.ref, helper->type.ref->next);
    vpushll(offset);
    gfunc_param_typed(helper->type.ref, helper->type.ref->next->next);
    gfunc_call(2);
    while (macro_stack && macro_stack != saved_macro) end_macro();
    macro_ptr = saved_ptr; tok = saved_token; tokc = saved_value;
    type = info_type; mk_pointer(&type);
    vset(&type, REG_IRET, 0); indir();
  } else {
    Sym *record = cpp_type_info_for_type(&type);
    vpushsym(&info_type, record);
    vtop->r |= VT_LVAL;
  }
  tok_str_free(operand);
}

static void cpp_dynamic_cast(CType *target)
{
  CType source, target_class, source_class;
  int source_tok, target_tok, offset, root, alignment;
  int reference = is_reference_type(target);
  Sym *source_name, *target_name;
  SValue operand;
  maybe_indir_reference();
  source = vtop->type;
  if (reference) {
    source_class = source;
    target_class = *pointed_type(target);
    if ((source_class.t & VT_BTYPE) != VT_STRUCT
        || (target_class.t & VT_BTYPE) != VT_STRUCT)
      cprime_error("dynamic_cast reference requires class types");
    source_tok = get_struct_type_name_tok(&source_class);
    target_tok = get_struct_type_name_tok(&target_class);
    if ((source_class.t & ~target_class.t) & (VT_CONSTANT | VT_VOLATILE))
      cprime_error("dynamic_cast cannot remove cv qualifiers");
    test_lvalue();
    mk_pointer(&vtop->type); gaddrof();
    {
      CType pointer = target_class;
      int checked = !cpp_rtti_public_base_offset(source_tok, target_tok, &offset);
      if (checked) cpp_noexcept_note_throw();
      mk_pointer(&pointer);
      cpp_dynamic_cast(&pointer);
      if (checked && !NODATA_WANTED) {
        operand = *vtop--;
        cpp_eh_push_helper("__cpc_dynamic_cast_reference");
        vpushv(&operand);
        gfunc_call(1);
        vset(&pointer, REG_IRET, 0);
      }
      indir();
    }
    vtop->r &= ~VT_CXX_PRVALUE;
    return;
  }
  if ((target->t & VT_BTYPE) != VT_PTR || (source.t & VT_BTYPE) != VT_PTR)
    cprime_error("dynamic_cast requires pointer or reference types");
  source_class = *pointed_type(&source);
  target_class = *pointed_type(target);
  if ((source_class.t & VT_BTYPE) != VT_STRUCT
      || ((target_class.t & VT_BTYPE) != VT_STRUCT && (target_class.t & VT_BTYPE) != VT_VOID))
    cprime_error("dynamic_cast requires class pointers or a void pointer target");
  if ((source_class.t & ~target_class.t) & (VT_CONSTANT | VT_VOLATILE))
    cprime_error("dynamic_cast cannot remove cv qualifiers");
  materialize_incomplete_template_type(&source_class);
  materialize_incomplete_template_type(&target_class);
  if (type_size(&source_class, &alignment) < 0
      || ((target_class.t & VT_BTYPE) != VT_VOID && type_size(&target_class, &alignment) < 0))
    cprime_error("dynamic_cast requires complete class types");
  source_tok = get_struct_type_name_tok(&source_class);
  target_tok = get_struct_type_name_tok(&target_class);
  if (target_tok && (source_tok == target_tok || class_has_base(source_tok, target_tok))) {
    if (!cpp_rtti_public_base_offset(source_tok, target_tok, &offset))
      cprime_error("dynamic_cast base is inaccessible or ambiguous");
    gen_cast(target);
    return;
  }
  if (!cpp_class_is_polymorphic(source_tok))
    cprime_error("dynamic_cast source type is not polymorphic");
  root = class_primary_virtual_root(source_tok);
  offset = root ? virtual_vptr_offset(source_tok, root) : -1;
  if (offset < 0) cprime_error("dynamic_cast source type is not polymorphic");
  if (NODATA_WANTED) {
    vpop(); vset(target, VT_CONST, 0);
    return;
  }
  source_name = cpp_rtti_type_name(&source_class);
  target_name = target_tok ? cpp_rtti_type_name(&target_class) : NULL;
  operand = *vtop--;
  cpp_eh_push_helper("__cpc_dynamic_cast");
  vpushv(&operand);
  vpushll(offset);
  vpushsym(&char_pointer_type, source_name);
  if (target_name) vpushsym(&char_pointer_type, target_name); else vpushi(0);
  gfunc_call(4);
  vset(target, REG_IRET, 0);
}
