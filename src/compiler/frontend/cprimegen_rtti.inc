#include "../../../include/runtime/cprime_rtti.h"

typedef struct CppRttiNode {
  int class_tok;
  int offset;
  int parent;
  int public_base;
} CppRttiNode;

static void cpp_rtti_collect(int class_tok, int offset, int parent, int access,
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
    if (base->class_tok == class_tok && base->field)
      cpp_rtti_collect(base->base_tok, offset + base->field->c, index,
                        base->access, nodes, count);
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

static Sym *cpp_rtti_table_descriptor(int class_tok, int root_tok)
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
  table.offset_to_top = -virtual_vptr_offset(class_tok, root_tok);
  table.count = count;
  descriptor = cpp_eh_data_symbol(&table, sizeof table, PTR_SIZE);
  cpp_eh_data_pointer(descriptor, offsetof(CpcRttiTable, nodes), entries);
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
    if (!cpp_rtti_public_base_offset(source_tok, target_tok, &offset))
      cprime_error("runtime-checked reference dynamic_cast is not yet supported");
    if ((source_class.t & ~target_class.t) & (VT_CONSTANT | VT_VOLATILE))
      cprime_error("dynamic_cast cannot remove cv qualifiers");
    test_lvalue();
    mk_pointer(&vtop->type); gaddrof();
    gen_cast(target);
    maybe_indir_call_result();
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
