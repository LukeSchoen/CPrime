#include "../../../include/runtime/cprime_rtti.h"

typedef struct CppTypeInfoRecord {
  int name_tok;
  Sym *symbol;
  struct CppTypeInfoRecord *next;
} CppTypeInfoRecord;
static CppTypeInfoRecord *cpp_type_info_records;

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

static void cpp_type_info_name_ex(CString *name, CType *original, int strip_cv)
{
  CType type = *original;
  if (is_reference_type(&type)) type = *pointed_type(&type);
  type.t &= ~(VT_STORAGE | VT_RVALUE_REFERENCE);
  if (strip_cv) type.t &= ~(VT_CONSTANT | VT_VOLATILE);
  if (type.t & VT_BITFIELD) type.t &= ~VT_STRUCT_MASK;
  if (type.t & VT_ARRAY) {
    char bound[48];
    snprintf(bound, sizeof(bound), "array%lld_", type.ref->c);
    cstr_cat(name, bound, -1);
    cpp_type_info_name_ex(name, pointed_type(&type), strip_cv);
  } else if ((type.t & VT_BTYPE) == VT_PTR) {
    if (type.t & VT_CONSTANT) cstr_cat(name, "const_", -1);
    if (type.t & VT_VOLATILE) cstr_cat(name, "volatile_", -1);
    cstr_cat(name, "ptr_", -1);
    cpp_type_info_name_ex(name, pointed_type(&type), 0);
  } else if (!append_type_mangle_string(name, &type))
    cprime_error("unsupported typeid type");
}

static Sym *cpp_type_info_for_type(CType *type)
{
  CType info_type;
  CString name;
  CppTypeInfoRecord *record;
  VirtualTableInfo *table;
  Sym *name_symbol, *field;
  int name_tok, size, alignment, field_offset;
  if (!cpp_type_info_class(&info_type)) return NULL;
  cstr_new(&name);
  cpp_type_info_name_ex(&name, type, 1);
  cstr_ccat(&name, 0);
  name_tok = tok_alloc_const(name.data);
  for (record = cpp_type_info_records; record; record = record->next)
    if (record->name_tok == name_tok) { cstr_free(&name); return record->symbol; }
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
    if (table->class_tok == get_struct_type_name_tok(&info_type)) {
      int offset = table->vptr_offset;
      table->referenced = 1;
      cpp_eh_data_pointer(record->symbol, offset, table->sym);
    }
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
  int source_tok, target_tok, offset, alignment, reference = is_reference_type(target);
  VirtualTableInfo *vt;
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
  offset = -1;
  if (!cpp_class_is_polymorphic(source_tok))
    cprime_error("dynamic_cast source type is not polymorphic");
  for (vt = virtual_table_infos; vt; vt = vt->next)
    if (vt->class_tok == source_tok) {
      offset = virtual_vptr_offset(source_tok, vt->root_tok);
      if (offset >= 0) break;
    }
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
