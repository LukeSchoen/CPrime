#include "../../../include/runtime/cprime_exception.h"

typedef struct CppEhNode {
  Sym *symbol;
  int hash_next;
  CpcEhCleanup data;
  Sym *action;
} CppEhNode;

typedef struct CppEhCatchEntry {
  CpcEhCatch data;
  Sym *type;
} CppEhCatchEntry;

typedef struct CppEhFunction {
  CpcEhState *states;
  int state_count;
  int state_capacity;
  CppEhNode *nodes;
  int node_count;
  int node_capacity;
  int node_buckets[1024];
  CppEhCatchEntry *catches;
  int catch_count;
  Sym **initializing;
  int initializing_count;
  Sym *cleanup_override;
  int has_cleanup_override;
  int enabled;
  Sym *function;
  unsigned flags;
} CppEhFunction;

static CppEhFunction cpp_eh_function;
static CType cpp_eh_void_type = { VT_VOID, NULL };

static int cpp_current_return_regs(CType *return_type, CType *register_type,
    int *alignment, int *register_size)
{
  Sym *function = cpp_eh_function.function;
  return cpp_function_return_regs(return_type, function ? function->type.ref : NULL,
      func_var, register_type, alignment, register_size);
}

static void new_scope(struct scope *o);
static void prev_scope(struct scope *o, int is_expr);
static void cpp_eh_parse_try(void);
static Sym *cpp_eh_register_local_cleanup(Sym *object, const char *helper);
static int saved_initializer_is_direct_constructor(TokenString *expression,
                                                    CType *type);
static void skip_or_save_block_ex(TokenString **str, int allow_eof);

static int cpp_return_operand_is_move_eligible(TokenString *expression)
{
  int first = tok_str_next_non_linenum_index(expression->str, expression->len, 0);
  int last = tok_str_prev_token_index(expression->str, expression->len,
                                      expression->len);
  Sym *object;
  while (first < last && expression->str[first] == '('
         && expression->str[last] == ')') {
    first = tok_str_next_non_linenum_index(expression->str, expression->len, first + 1);
    last = tok_str_prev_token_index(expression->str, expression->len, last);
  }
  if (first != last || first < 0 || expression->str[first] < TOK_UIDENT)
    return 0;
  object = sym_find(expression->str[first]);
  return object && object->sym_scope && !(object->type.t & (VT_STATIC | VT_VOLATILE))
         && !is_reference_type(&object->type)
         && (object->type.t & VT_BTYPE) == VT_STRUCT;
}

static int cpp_parse_class_return_expression(void)
{
  CType register_type;
  int alignment, register_size, saved_tok, move_eligible;
  CValue saved_tokc;
  TokenString *expression, *parent, macro;
  const int *parent_pointer;
  CppConstructionDestination construction, *saved_destination;
  if (!is_cpp_translation_unit() || (func_vt.t & VT_BTYPE) != VT_STRUCT
      || cpp_current_return_regs(&func_vt, &register_type, &alignment, &register_size) != 0)
    return 0;
  skip_or_save_block_ex(&expression, 1);
  while (tok == ',') {
    TokenString *part;
    --expression->len;
    tok_str_add(expression, ','); next();
    skip_or_save_block_ex(&part, 1);
    tok_str_append(expression, part);
    tok_str_free(part);
  }
  construction.type = func_vt;
  move_eligible = cpp_return_operand_is_move_eligible(expression);
  construction.storage = VT_LLOCAL | VT_LVAL;
  construction.address = func_vc;
  construction.available = saved_initializer_is_direct_constructor(expression, &func_vt);
  construction.conditional_marker = cpp_prepare_conditional_destination(expression);
  saved_destination = cpp_construction_destination;
  cpp_construction_destination = &construction;
  saved_tok = tok; saved_tokc = tokc;
  parent = macro_stack; parent_pointer = macro_ptr;
  macro = *expression;
  begin_macro(&macro, 0); next(); gexpr();
  /* A returned automatic object (including a by-value parameter) participates
     in constructor overload resolution as an rvalue when copy elision is not
     performed. The named object still owns its normal scope cleanup. */
  if (move_eligible && (vtop->type.t & VT_BTYPE) == VT_STRUCT)
    vtop->type.t |= VT_RVALUE_REFERENCE;
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_pointer;
  tok = saved_tok; tokc = saved_tokc;
  cpp_construction_destination = saved_destination;
  tok_str_free(expression);
  return 1;
}

static TokenString *cpp_eh_save_throw_operand(void)
{
  TokenString *expression = tok_str_alloc();
  int depth = 0, angles = 0, conditional = 0, previous = 0, before_previous = 0;
  for (;;) {
    int current = tok;
    if (!depth && !angles && (current == ',' || current == ';'
        || current == ')' || current == ']' || current == '}' || current == TOK_EOF)) break;
    if (!depth && !angles && current == ':') {
      /* A scope qualifier is part of the operand; an unmatched conditional
         colon belongs to the expression containing this throw. */
      next();
      if (tok == ':') {
        tok_str_add(expression, ':'); tok_str_add(expression, ':');
        next(); previous = ':'; continue;
      }
      unget_tok(tok); tok = ':';
      if (!conditional) break;
      --conditional;
    }
    if (current == TOK_EOF) cprime_error("unterminated throw expression");
    tok_str_add_tok(expression);
    next();
    if ((current == TOK_LT || current == '<')
        && (angles || (previous >= TOK_UIDENT
            && (find_template_def(previous) || is_template_keyword_tok(before_previous))))) ++angles;
    else if ((current == TOK_SAR || current == TOK_SHR) && angles) {
      if (--angles) --angles;
    } else if (current == TOK_GT && angles) --angles;
    else if (current == '(' || current == '[' || current == '{' || is_cpp_lambda_marker(current)) ++depth;
    else if (current == ')' || current == ']' || current == '}') --depth;
    else if (!depth && !angles && current == '?') ++conditional;
    if (current != TOK_LINENUM && !TOK_HAS_VALUE(current)) {
      before_previous = previous; previous = current;
    }
  }
  tok_str_add(expression, TOK_EOF);
  return expression;
}

static void cpp_eh_replay_operand(TokenString *expression)
{
  TokenString macro = *expression, *parent = macro_stack;
  const int *parent_pointer = macro_ptr;
  int saved_tok = tok;
  CValue saved_tokc = tokc;
  begin_macro(&macro, 0); next(); expr_eq();
  while (macro_stack && macro_stack != parent) end_macro();
  macro_ptr = parent_pointer;
  tok = saved_tok; tokc = saved_tokc;
}

typedef struct CppEhBaseEntry {
  int class_tok;
  int offset;
  int paths;
  int public_path;
} CppEhBaseEntry;

static void cpp_eh_collect_bases(int class_tok, int offset, int public_path,
                                CppEhBaseEntry **entries, int *count)
{
  ClassBaseInfo *base;
  for (base = class_base_candidates(class_tok); base; base = base->bucket_next) {
    int i, accessible;
    CppEhBaseEntry *entry;
    if (base->class_tok != class_tok || !base->field) continue;
    accessible = public_path && base->access == 0;
    for (i = 0; i < *count; ++i)
      if ((*entries)[i].class_tok == base->base_tok) break;
    if (i == *count) {
      *entries = cprime_realloc(*entries, (++*count) * sizeof(**entries));
      memset(*entries + i, 0, sizeof(**entries));
      (*entries)[i].class_tok = base->base_tok;
    }
    entry = *entries + i;
    ++entry->paths;
    entry->public_path |= accessible;
    entry->offset = offset + base->field->c;
    cpp_eh_collect_bases(base->base_tok, entry->offset, accessible, entries, count);
  }
}

static void cpp_eh_function_begin(Sym *sym)
{
  Sym *parameter;
  memset(&cpp_eh_function, 0, sizeof cpp_eh_function);
  cpp_eh_function.enabled = is_cpp_translation_unit();
  cpp_eh_function.function = sym;
  if (cpp_eh_function.enabled)
    for (parameter = sym->type.ref->next; parameter; parameter = parameter->next)
      if ((!(parameter->v & ~SYM_FIELD)
           || (parameter->v & ~SYM_FIELD) >= SYM_FIRST_ANOM)
          && (parameter->type.t & VT_BTYPE) == VT_STRUCT
          && resolve_autodtor_func(&parameter->type)) {
        char name[64];
        snprintf(name, sizeof name, "__cpc_parameter_lifetime_%u", anon_sym++);
        parameter->v = tok_alloc_const(name);
      }
  if (sym->type.ref && (sym->type.ref->f.func_noexcept
      || (sym->type.ref->f.func_cxx_destructor
          && !sym->type.ref->f.func_noexcept_specified
          && sym->type.ref->next
          && class_destructor_is_noexcept(pointed_type(&sym->type.ref->next->type)))))
    cpp_eh_function.flags |= CPC_EH_FUNCTION_NOEXCEPT;
}

static void cpp_eh_register_subobject_cleanup(CType *type, Sym *this_symbol,
                                              int offset)
{
  Sym *destructor, *object, *cleanup;
  CType pointer;
  int index, size, align, slot;
  if ((type->t & VT_ARRAY) && type->ref) {
    CType *element = pointed_type(type);
    size = type_size(element, &align);
    for (index = 0; index < type->ref->c; ++index)
      cpp_eh_register_subobject_cleanup(element, this_symbol, offset + index * size);
    return;
  }
  destructor = resolve_autodtor_func(type);
  if (!destructor) return;
  pointer = *type;
  mk_pointer(&pointer);
  loc = (loc - PTR_SIZE) & -PTR_SIZE;
  slot = loc;
  vset(&pointer, VT_LOCAL | VT_LVAL, slot);
  vset(&this_symbol->type, this_symbol->r, this_symbol->c);
  vtop->sym = this_symbol;
  vtop->type = char_pointer_type;
  if (offset) { vpushi(offset); gen_op('+'); }
  vtop->type = pointer;
  vstore(); vpop();
  object = sym_push2(&local_stack, SYM_FIELD, 0, slot);
  object->type = *type;
  object->r = VT_LLOCAL | VT_LVAL;
  cleanup = sym_push2(&all_cleanups, SYM_FIELD | ++cur_scope->cl.n, 0, 0);
  cleanup->cleanup_sym = object;
  cleanup->cleanup_func = destructor;
  cleanup->next = cur_scope->cl.s;
  cur_scope->cl.s = cleanup;
}

/* Subobjects outlive a destructor's body locals. Putting their ordinary
   cleanups in the outer body scope gives returns and exceptional exits the
   same reverse construction order as normal fallthrough. */
static void cpp_eh_enter_function_body(void)
{
  Sym *function = cpp_eh_function.function;
  Sym *this_symbol, *field, *parameter;
  CType *type;
  if (!cpp_eh_function.enabled || local_scope != 1 || !function) return;
  for (parameter = function->type.ref->next; parameter; parameter = parameter->next) {
    Sym *destructor, *object, *cleanup;
    if ((parameter->type.t & VT_BTYPE) != VT_STRUCT) continue;
    destructor = resolve_autodtor_func(&parameter->type);
    object = sym_find(parameter->v);
    if (!destructor || !object) continue;
    cleanup = sym_push2(&all_cleanups, SYM_FIELD | ++cur_scope->cl.n, 0, 0);
    cleanup->cleanup_sym = object; cleanup->cleanup_func = destructor;
    cleanup->next = cur_scope->cl.s; cur_scope->cl.s = cleanup;
  }
  if (!function->type.ref->f.func_cxx_destructor) return;
  this_symbol = sym_find(function->type.ref->next->v);
  if (!this_symbol) cprime_error("destructor requires its implicit object parameter");
  type = pointed_type(&this_symbol->type);
  if (IS_UNION(type->t)) return;
  for (field = type->ref->next; field; field = field->next)
    cpp_eh_register_subobject_cleanup(&field->type, this_symbol, field->c);
}

static void cpp_eh_constructed_member(void)
{
  CType type, pointer;
  Sym *destructor, *object, *cleanup;
  int slot;
  skip('('); expr_eq(); skip(')'); skip(';');
  maybe_indir_reference();
  type = vtop->type;
  destructor = resolve_autodtor_func(&type);
  if (!destructor || nocode_wanted) { vpop(); return; }
  pointer = type; mk_pointer(&pointer);
  mk_pointer(&vtop->type); gaddrof();
  loc = (loc - PTR_SIZE) & -PTR_SIZE; slot = loc;
  vset(&pointer, VT_LOCAL | VT_LVAL, slot); vswap(); vstore(); vpop();
  object = sym_push2(&local_stack, SYM_FIELD, 0, slot);
  object->type = type; object->r = VT_LLOCAL | VT_LVAL;
  cleanup = sym_push2(&all_cleanups, SYM_FIELD | ++cur_scope->cl.n, 0, 0);
  cleanup->a.cleanup_exception_only = 1;
  cleanup->cleanup_sym = object; cleanup->cleanup_func = destructor;
  cleanup->next = cur_scope->cl.s; cur_scope->cl.s = cleanup;
}

static void cpp_eh_constructor_body(void)
{
  Sym *function = cpp_eh_function.function;
  Sym *object;
  SValue pointer;
  CType type;
  skip(';');
  if (!function || !function->type.ref->next)
    cprime_error("constructor boundary requires an object parameter");
  object = sym_find(function->type.ref->next->v);
  if (!object || (object->type.t & VT_BTYPE) != VT_PTR)
    cprime_error("constructor boundary requires an object parameter");
  type = *pointed_type(&object->type);
  vset(&object->type, object->r, object->c); vtop->sym = object;
  pointer = *vtop--;
  initialize_virtual_tables_for_pointer(&type, &pointer, 0);
}

static void cpp_eh_function_end(void)
{
  cprime_free(cpp_eh_function.states);
  cprime_free(cpp_eh_function.nodes);
  cprime_free(cpp_eh_function.catches);
  cprime_free(cpp_eh_function.initializing);
  memset(&cpp_eh_function, 0, sizeof cpp_eh_function);
}

static void cpp_eh_mark_local_initializing(Sym *symbol)
{
  CppEhFunction *eh = &cpp_eh_function;
  if (!eh->enabled || nocode_wanted || !symbol) return;
  eh->initializing = cprime_realloc(eh->initializing,
      (eh->initializing_count + 1) * sizeof(*eh->initializing));
  eh->initializing[eh->initializing_count++] = symbol;
}

static void cpp_eh_finish_local_initialization(Sym *symbol)
{
  CppEhFunction *eh = &cpp_eh_function;
  int i;
  for (i = eh->initializing_count - 1; i >= 0; --i)
    if (eh->initializing[i] == symbol) {
      memmove(eh->initializing + i, eh->initializing + i + 1,
          (--eh->initializing_count - i) * sizeof(*eh->initializing));
      return;
    }
}

static void cpp_eh_set_cleanup_state(Sym *state)
{
  cpp_eh_function.cleanup_override = state;
  cpp_eh_function.has_cleanup_override = 1;
}

static void cpp_eh_restore_cleanup_state(void)
{
  cpp_eh_function.has_cleanup_override = 0;
}

static void cpp_eh_register_parameter_copy(CType *type, int storage, int address)
{
  Sym *destructor, *object, *cleanup;
  if (!cpp_eh_function.enabled || nocode_wanted) return;
  destructor = resolve_autodtor_func(type);
  if (!destructor) return;
  object = sym_push2(&local_stack, SYM_FIELD, 0, address);
  object->type = *type; object->r = storage;
  cleanup = sym_push2(&all_cleanups, SYM_FIELD | ++cur_scope->cl.n, 0, 0);
  cleanup->cleanup_sym = object; cleanup->cleanup_func = destructor;
  cleanup->next = cur_scope->cl.s; cur_scope->cl.s = cleanup;
}

static int cpp_eh_cleanup_index(Sym *symbol)
{
  CppEhFunction *eh = &cpp_eh_function;
  CppEhNode *node;
  int i, parent, bucket;
  if (!symbol) return -1;
  for (i = 0; i < eh->initializing_count; ++i)
    if (eh->initializing[i] == symbol->cleanup_sym)
      return cpp_eh_cleanup_index(symbol->next);
  bucket = ((uintptr_t)symbol >> 4) & 1023;
  for (i = eh->node_buckets[bucket]; i; i = eh->nodes[i - 1].hash_next)
    if (eh->nodes[i - 1].symbol == symbol) return i - 1;
  parent = cpp_eh_cleanup_index(symbol->next);
  if (!symbol->cleanup_func || !symbol->cleanup_sym) return parent;
  i = eh->node_count++;
  if (eh->node_count > eh->node_capacity) {
    eh->node_capacity = eh->node_capacity ? eh->node_capacity * 2 : 32;
    eh->nodes = cprime_realloc(eh->nodes, eh->node_capacity * sizeof(*eh->nodes));
  }
  node = eh->nodes + i;
  memset(node, 0, sizeof(*node));
  node->symbol = symbol;
  node->hash_next = eh->node_buckets[bucket];
  eh->node_buckets[bucket] = i + 1;
  node->action = symbol->cleanup_func;
  node->data.parent = parent;
  node->data.object_offset = symbol->cleanup_sym->c;
  node->data.active_flag_offset = cpp_temp_cleanup_active_offset(symbol);
  if ((symbol->cleanup_sym->r & VT_VALMASK) == VT_LLOCAL)
    node->data.flags |= CPC_EH_CLEANUP_INDIRECT;
  return i;
}

static void cpp_eh_note_call(void)
{
  CppEhFunction *eh = &cpp_eh_function;
  CpcEhState *state;
  int cleanup;
  if (!eh->enabled || nocode_wanted || !cur_scope) return;
  cleanup = cpp_eh_cleanup_index(eh->has_cleanup_override
      ? eh->cleanup_override : cur_scope->cl.s);
  if (cleanup < 0) return;
  if (eh->state_count == eh->state_capacity) {
    eh->state_capacity = eh->state_capacity ? eh->state_capacity * 2 : 32;
    eh->states = cprime_realloc(eh->states, eh->state_capacity * sizeof(*eh->states));
  }
  state = eh->states + eh->state_count++;
  state->begin_rva = ind - 1;
  state->end_rva = ind;
  state->cleanup = cleanup;
}

static void cpp_eh_push_helper(const char *name)
{
  vpush_helper_func(tok_alloc_const(name));
}

static void cpp_eh_push_local_address(int offset)
{
  vset(&char_pointer_type, VT_LOCAL, offset);
}

static Sym *cpp_eh_data_symbol(const void *data, unsigned size, unsigned align)
{
  unsigned offset = section_add(data_section, size, align);
  Sym *symbol;
  if (data) memcpy(data_section->data + offset, data, size);
  symbol = sym_push2(&global_stack, anon_sym++, char_pointer_type.t | VT_STATIC, 0);
  symbol->type.ref = char_pointer_type.ref;
  symbol->r = VT_CONST | VT_SYM;
  put_extern_sym(symbol, data_section, offset, size);
  return symbol;
}

static void cpp_eh_data_pointer(Sym *owner, unsigned member_offset, Sym *target)
{
  if (target)
    greloca(data_section, target, elfsym(owner)->st_value + member_offset,
            R_X86_64_64, 0);
}

static Sym *cpp_eh_type_descriptor(CType *original)
{
  CType type = *original;
  CpcEhType descriptor;
  CString name;
  Sym *symbol, *name_symbol, *pointee = NULL, *bases_symbol = NULL;
  CppEhBaseEntry *bases = NULL;
  Sym **base_types = NULL;
  int base_count = 0, i;
  int align;
  if (is_reference_type(&type)) type = *pointed_type(&type);
  memset(&descriptor, 0, sizeof descriptor);
  descriptor.qualifiers = ((type.t & VT_CONSTANT) ? CPC_EH_CONST : 0)
                        | ((type.t & VT_VOLATILE) ? CPC_EH_VOLATILE : 0);
  type.t &= ~(VT_STORAGE | VT_CONSTANT | VT_VOLATILE);
  descriptor.size = type_size(&type, &align);
  if ((int)descriptor.size < 0) cprime_error("exception type must be complete");
  descriptor.kind = (type.t & VT_BTYPE) == VT_STRUCT ? CPC_EH_CLASS
      : (type.t & VT_BTYPE) == VT_PTR ? CPC_EH_POINTER
      : (type.t & VT_BTYPE) == VT_VOID ? CPC_EH_VOID
      : (type.t & VT_BTYPE) == VT_FUNC ? CPC_EH_FUNCTION
      : ((type.t & VT_BTYPE) == VT_NULLPTR) ? CPC_EH_NULLPTR : CPC_EH_SCALAR;
  if (descriptor.kind == CPC_EH_POINTER) pointee = cpp_eh_type_descriptor(pointed_type(&type));
  if (descriptor.kind == CPC_EH_CLASS) {
    cpp_eh_collect_bases(get_struct_type_name_tok(&type), 0, 1, &bases, &base_count);
    for (i = 0; i < base_count; ++i) {
      CType base_type;
      if (bases[i].paths != 1 || !bases[i].public_path) continue;
      if (!make_class_type_from_tok(&base_type, bases[i].class_tok)) continue;
      base_types = cprime_realloc(base_types, (descriptor.base_count + 1) * sizeof(*base_types));
      base_types[descriptor.base_count] = cpp_eh_type_descriptor(&base_type);
      bases[descriptor.base_count++] = bases[i];
    }
    if (descriptor.base_count) {
      bases_symbol = cpp_eh_data_symbol(NULL, descriptor.base_count * sizeof(CpcEhBase), PTR_SIZE);
      for (i = 0; i < descriptor.base_count; ++i) {
        CpcEhBase base;
        memset(&base, 0, sizeof base);
        base.offset = bases[i].offset;
        memcpy(data_section->data + elfsym(bases_symbol)->st_value + i * sizeof base, &base, sizeof base);
        cpp_eh_data_pointer(bases_symbol, i * sizeof base + offsetof(CpcEhBase, type), base_types[i]);
      }
    }
  }
  cstr_new(&name);
  append_type_mangle_string(&name, &type);
  cstr_ccat(&name, 0);
  name_symbol = cpp_eh_data_symbol(name.data, name.size, 1);
  cstr_free(&name);
  symbol = cpp_eh_data_symbol(&descriptor, sizeof descriptor, PTR_SIZE);
  cpp_eh_data_pointer(symbol, offsetof(CpcEhType, name), name_symbol);
  cpp_eh_data_pointer(symbol, offsetof(CpcEhType, pointee), pointee);
  cpp_eh_data_pointer(symbol, offsetof(CpcEhType, bases), bases_symbol);
  cprime_free(base_types);
  cprime_free(bases);
  return symbol;
}

static void cpp_eh_throw_expression(void)
{
  CType type, pointer_type;
  Sym *descriptor, *destructor, *discard_object, *discard_cleanup;
  SValue source, destination;
  int size, align, source_slot, object_slot, source_pointer_slot;
  int source_reg;
  TokenString *operand;
  int direct = 0;
  next();
  if (tok == ';' || tok == ')' || tok == ',' || tok == ':' || tok == '}') {
    cpp_eh_push_helper("__cpc_eh_rethrow");
    gfunc_call(0);
    vset(&cpp_eh_void_type, VT_CONST | VT_THROW, 0);
    CODE_OFF();
    return;
  }
  if (nocode_wanted) {
    expr_eq(); vpop(); vset(&cpp_eh_void_type, VT_CONST | VT_THROW, 0);
    return;
  }
  operand = cpp_eh_save_throw_operand();
  infer_saved_arg_types(&operand, &type, 1);
  if (is_reference_type(&type)) type = *pointed_type(&type);
  type.t &= ~(VT_STORAGE | VT_CONSTANT | VT_VOLATILE | VT_RVALUE_REFERENCE);
  direct = (type.t & VT_BTYPE) == VT_STRUCT
        && saved_initializer_is_direct_constructor(operand, &type);
  if (direct) {
    size = type_size(&type, &align);
    if (size < 0) cprime_error("exception type must be complete");
    goto allocate_exception;
  }
  cpp_eh_replay_operand(operand);
  maybe_indir_reference();
  type = vtop->type;
  type.t &= ~(VT_STORAGE | VT_CONSTANT | VT_VOLATILE);
  if (type.t & VT_ARRAY) {
    type.t &= ~VT_ARRAY;
    vtop->type = type;
  }
  if ((type.t & VT_BTYPE) == VT_VOID) cprime_error("cannot throw a void expression");
  size = type_size(&type, &align);
  if (size < 0) cprime_error("exception type must be complete");
  /* Preserve evaluation across allocation without copying a class twice. */
  if (!(vtop->r & VT_LVAL)) {
    loc = (loc - size) & -align;
    source_slot = loc;
    source_reg = VT_CONST;
    vset(&type, VT_LOCAL | VT_LVAL, source_slot);
    vtop->r2 = source_reg;
    vswap(); vstore();
    vpop();
    vset(&type, VT_LOCAL | VT_LVAL, source_slot);
    vtop->r2 = source_reg;
  }
  mk_pointer(&vtop->type); gaddrof();
  pointer_type = vtop->type;
  loc = (loc - PTR_SIZE) & -PTR_SIZE;
  source_pointer_slot = loc;
  vset(&pointer_type, VT_LOCAL | VT_LVAL, source_pointer_slot);
  vswap(); vstore(); vpop();
  vset(&pointer_type, VT_LOCAL | VT_LVAL, source_pointer_slot);
  source = *vtop--;
allocate_exception:
  descriptor = cpp_eh_type_descriptor(&type);
  destructor = resolve_autodtor_func(&type);
  cpp_eh_push_helper("__cpc_eh_allocate");
  vpushsym(&char_pointer_type, descriptor);
  vpushi(align);
  if (destructor) vpushsym(&destructor->type, destructor); else vpushi(0);
  gfunc_call(3);
  pointer_type = type; mk_pointer(&pointer_type);
  loc = (loc - PTR_SIZE) & -PTR_SIZE;
  object_slot = loc;
  vset(&pointer_type, VT_LOCAL | VT_LVAL, object_slot);
  vset(&pointer_type, REG_IRET, 0);
  vstore(); vpop();
  vset(&pointer_type, VT_LOCAL | VT_LVAL, object_slot);
  destination = *vtop--;
  discard_object = sym_push(anon_sym++, &type, VT_LLOCAL | VT_LVAL, object_slot);
  discard_cleanup = cpp_eh_register_local_cleanup(discard_object, "__cpc_eh_discard");
  if (direct) {
    CppConstructionDestination construction, *saved = cpp_construction_destination;
    construction.type = type;
    construction.storage = VT_LLOCAL | VT_LVAL;
    construction.address = object_slot;
    construction.available = 1;
    construction.conditional_marker = cpp_prepare_conditional_destination(operand);
    cpp_construction_destination = &construction;
    cpp_eh_replay_operand(operand);
    cpp_construction_destination = saved;
    if (construction.available)
      cprime_internal_error("exception construction did not consume destination");
    vpop();
  } else if ((type.t & VT_BTYPE) == VT_STRUCT && !resolve_copy_constructor_func(&type, &type))
    copy_construct_struct_memberwise_from_base_ptr(&type, &destination, &source, 0, 0);
  else
    copy_construct_one_lvalue_pair(&type, &destination, &source, 0, 0);
  tok_str_free(operand);
  cur_scope->cl.s = discard_cleanup->next;
  --cur_scope->cl.n;
  cpp_eh_push_helper("__cpc_eh_throw");
  vpushv(&destination);
  gfunc_call(1);
  vset(&cpp_eh_void_type, VT_CONST | VT_THROW, 0);
  CODE_OFF();
}

static Sym *cpp_eh_register_local_cleanup(Sym *object, const char *helper)
{
  Sym *cleanup = sym_push2(&all_cleanups, SYM_FIELD | ++cur_scope->cl.n, 0, 0);
  cleanup->cleanup_sym = object;
  cleanup->cleanup_func = external_helper_sym(tok_alloc_const(helper));
  cleanup->next = cur_scope->cl.s;
  cur_scope->cl.s = cleanup;
  return cleanup;
}

static int cpp_array_cookie_size(CType *type, Sym *destructor)
{
  int alignment;
  if (!destructor) return 0;
  type_size(type, &alignment);
  return alignment > PTR_SIZE ? alignment : PTR_SIZE;
}

static Sym *cpp_eh_register_array_cleanup(int pointer_slot, int count_slot,
                                          int element_size, Sym *destructor,
                                          int *context_slot)
{
  CType count_type = { VT_LLONG | VT_UNSIGNED, NULL };
  Sym *object;
  int context;
  loc = (loc - sizeof(CpcEhArray)) & -PTR_SIZE;
  context = loc;
  vset(&char_pointer_type, VT_LOCAL | VT_LVAL, context + offsetof(CpcEhArray, data));
  vset(&char_pointer_type, VT_LOCAL | VT_LVAL, pointer_slot);
  vstore(); vpop();
  vset(&count_type, VT_LOCAL | VT_LVAL, context + offsetof(CpcEhArray, count));
  if (count_slot) vset(&count_type, VT_LOCAL | VT_LVAL, count_slot);
  else vpushi(0);
  vstore(); vpop();
  vset(&count_type, VT_LOCAL | VT_LVAL, context + offsetof(CpcEhArray, element_size));
  vpushi(element_size); vstore(); vpop();
  vset(&char_pointer_type, VT_LOCAL | VT_LVAL, context + offsetof(CpcEhArray, destructor));
  vpushsym(&destructor->type, destructor);
  vtop->type = char_pointer_type;
  vstore(); vpop();
  object = sym_push2(&local_stack, SYM_FIELD, 0, context);
  object->type = cpp_eh_void_type; object->r = VT_LOCAL | VT_LVAL;
  *context_slot = context;
  return cpp_eh_register_local_cleanup(object, "__cpc_eh_destroy_array");
}

static void cpp_new_array(CType *type, Sym *allocation, int size, int alignment)
{
  CType pointer = *type, count_type = { VT_LLONG | VT_UNSIGNED, NULL };
  Sym *destructor = resolve_autodtor_func(type), *constructor;
  Sym *old_cleanup, *object, *cleanup;
  int old_depth, count_slot, pointer_slot, allocation_slot;
  int cookie = cpp_array_cookie_size(type, destructor), context = 0;
  int index_slot, loop, done, value_initialize = 0, null_allocation_jump = 0;
  (void)alignment;
  next(); expr_eq(); skip(']');
  if ((vtop->type.t & VT_BTYPE) == VT_STRUCT)
    try_call_cpp_conversion_operator(&count_type, 0);
  if (!is_integer_btype(vtop->type.t & VT_BTYPE))
    cprime_error("array new bound requires an integral expression");
  if (tok == '[') {
    TokenString *dimensions = tok_str_alloc();
    int extent_size, extent_align;
    add_ctype_tokens(dimensions, type);
    while (tok == '[') skip_or_save_balanced_tokens(dimensions);
    tok_str_add(dimensions, TOK_EOF);
    if (!template_probe_type(dimensions, 6, current_namespace_scope_tok(), &pointer))
      cprime_error("invalid array new dimensions");
    tok_str_free(dimensions);
    extent_size = type_size(&pointer, &extent_align);
    if (extent_size < 0 || size <= 0) cprime_error("invalid array new element type");
    vpushi(extent_size / size); gen_op('*');
  }
  mk_pointer(&pointer);
  if ((type->t & VT_BTYPE) == VT_STRUCT && type->ref
      && type->ref->a.cpp_user_constructor && !resolve_autoctor_func(type))
    cprime_error("array new requires a viable default constructor");
  if (NOEVAL_WANTED) {
    if (tok == '(' || tok == '{') {
      int end = tok == '(' ? ')' : '}';
      next(); skip(end);
    }
    vpop(); vset(&pointer, VT_CONST, 0);
    return;
  }
  old_cleanup = cur_scope->cl.s;
  old_depth = cur_scope->cl.n;
  loc = (loc - 8) & -8; count_slot = loc;
  vset(&count_type, VT_LOCAL | VT_LVAL, count_slot);
  vswap(); vstore(); vpop();
  if (tok == '(' || tok == '{') {
    int end = tok == '(' ? ')' : '}';
    next(); skip(end); value_initialize = 1;
  }
  vpushsym(&allocation->type, allocation);
  vset(&count_type, VT_LOCAL | VT_LVAL, count_slot);
  vpushi(size); gen_op('*');
  if (cookie) { vpushi(cookie); gen_op('+'); }
  gfunc_param_typed(allocation->type.ref, allocation->type.ref->next);
  gfunc_call(1);
  loc = (loc - PTR_SIZE) & -PTR_SIZE; allocation_slot = loc;
  vset(&pointer, VT_LOCAL | VT_LVAL, allocation_slot);
  vpush(&pointer); PUT_R_RET(vtop, pointer.t);
  vstore(); vpop();
  if (allocation->type.ref->f.func_noexcept) {
    vset(&pointer, VT_LOCAL | VT_LVAL, allocation_slot);
    null_allocation_jump = gvtst(1, 0);
  }
  pointer_slot = allocation_slot;
  if (cookie) {
    loc = (loc - PTR_SIZE) & -PTR_SIZE; pointer_slot = loc;
    vset(&pointer, VT_LOCAL | VT_LVAL, pointer_slot);
    vset(&char_pointer_type, VT_LOCAL | VT_LVAL, allocation_slot);
    vpushi(cookie); gen_op('+'); vtop->type = pointer;
    vstore(); vpop();
    vset(&char_pointer_type, VT_LOCAL | VT_LVAL, pointer_slot);
    vpushi(PTR_SIZE); gen_op('-');
    vtop->type = count_type; vtop->r |= VT_LVAL;
    vset(&count_type, VT_LOCAL | VT_LVAL, count_slot);
    vstore(); vpop();
  }
  if (!type_requires_class_construction(type)
      && (!value_initialize || (type->t & VT_BTYPE) != VT_STRUCT)) {
    if (value_initialize) {
      vpush_helper_func(tok_alloc_const("memset"));
      vset(&pointer, VT_LOCAL | VT_LVAL, pointer_slot);
      vpushi(0);
      vset(&count_type, VT_LOCAL | VT_LVAL, count_slot);
      vpushi(size); gen_op('*');
      gfunc_call(3);
    }
    gsym(null_allocation_jump);
    vset(&pointer, VT_LOCAL | VT_LVAL, allocation_slot);
    if (cookie) {
      int null_result;
      vtop->type = char_pointer_type; gv(RC_INT);
      vdup(); null_result = gvtst(1, 0);
      vpushi(cookie); gen_op('+');
      gsym(null_result); vtop->type = pointer;
    }
    return;
  }
  object = sym_push2(&local_stack, SYM_FIELD, 0, allocation_slot);
  object->type = cpp_eh_void_type; object->r = VT_LLOCAL | VT_LVAL;
  cleanup = cpp_eh_register_local_cleanup(object, "free");
  cleanup->a.cleanup_exception_only = 1;
  if (destructor) {
    cleanup = cpp_eh_register_array_cleanup(pointer_slot, 0, size, destructor, &context);
    cleanup->a.cleanup_exception_only = 1;
    index_slot = context + offsetof(CpcEhArray, count);
  } else {
    loc = (loc - 8) & -8; index_slot = loc;
    vset(&count_type, VT_LOCAL | VT_LVAL, index_slot);
    vpushi(0); vstore(); vpop();
  }
  constructor = resolve_autoctor_func(type);
  loop = ind;
  vset(&count_type, VT_LOCAL | VT_LVAL, index_slot);
  vset(&count_type, VT_LOCAL | VT_LVAL, count_slot);
  gen_op(TOK_LT); done = gvtst(1, 0);
  vset(&char_pointer_type, VT_LOCAL | VT_LVAL, pointer_slot);
  vset(&count_type, VT_LOCAL | VT_LVAL, index_slot);
  vpushi(size); gen_op('*'); gen_op('+');
  vtop->type = *type; vtop->r |= VT_LVAL;
  if (constructor)
    call_lifecycle_constructor_noargs_expr(type, constructor);
  else if ((type->t & VT_BTYPE) == VT_STRUCT) {
    SValue base_pointer;
    mk_pointer(&vtop->type); gaddrof(); save_regs(0);
    base_pointer = *vtop;
    call_lifecycle_constructor_members_base_ptr(type, &base_pointer, 0);
    initialize_virtual_tables_for_pointer(type, &base_pointer, 0);
    vpop();
  } else {
    if (value_initialize) { vpushi(0); vstore(); }
    vpop();
  }
  vset(&count_type, VT_LOCAL | VT_LVAL, index_slot);
  vdup(); vpushi(1); gen_op('+'); vstore(); vpop();
  gjmp_addr(loop); gsym(done);
  cur_scope->cl.s = old_cleanup; cur_scope->cl.n = old_depth;
  gsym(null_allocation_jump);
  vset(&pointer, VT_LOCAL | VT_LVAL, allocation_slot);
  if (cookie) {
    int null_result;
    vtop->type = char_pointer_type; gv(RC_INT);
    vdup(); null_result = gvtst(1, 0);
    vpushi(cookie); gen_op('+');
    gsym(null_result); vtop->type = pointer;
  }
}

static void cpp_delete_pointer(int array_delete)
{
  CType pointer = vtop->type, object_type = *pointed_type(&pointer);
  Sym *destructor;
  Sym *free_function, *cleanup = NULL, *object;
  VirtualMethodInfo *virtual_destructor = NULL;
  int pointer_slot, allocation_slot, skip_null, class_name, cleanup_depth;
  int free_name = tok_alloc_const("free");
  /* Preserve the GNU pointer-to-array delete extension. Such a pointer
     denotes array allocation even when the expression omits []. */
  if (object_type.t & VT_ARRAY) array_delete = 1;
  if (array_delete)
    while (object_type.t & VT_ARRAY) object_type = *pointed_type(&object_type);
  destructor = resolve_autodtor_func(&object_type);
  if (!array_delete && destructor && class_has_virtual_destructor(&object_type))
    virtual_destructor = find_virtual_method_for_symbol(
        get_struct_type_name_tok(&object_type), destructor->v);
  free_function = global_symbol_find(free_name);
  if (!free_function || IS_ASM_SYM(free_function))
    free_function = external_global_sym(free_name, &func_old_type);
  loc = (loc - PTR_SIZE) & -PTR_SIZE;
  pointer_slot = loc;
  vset(&pointer, VT_LOCAL | VT_LVAL, pointer_slot);
  vswap(); vstore(); vpop();
  vset(&pointer, VT_LOCAL | VT_LVAL, pointer_slot);
  skip_null = gvtst(1, 0);
  allocation_slot = pointer_slot;
  if (array_delete && destructor) {
    loc = (loc - PTR_SIZE) & -PTR_SIZE;
    allocation_slot = loc;
    vset(&char_pointer_type, VT_LOCAL | VT_LVAL, allocation_slot);
    vset(&char_pointer_type, VT_LOCAL | VT_LVAL, pointer_slot);
    vpushi(cpp_array_cookie_size(&object_type, destructor)); gen_op('-');
    vstore(); vpop();
  }
  if (virtual_destructor) {
    int root_offset, field_offset;
    class_name = get_struct_type_name_tok(&object_type);
    field_offset = virtual_vptr_offset(class_name, virtual_destructor->root_tok);
    if (!class_subobject_offset(class_name, virtual_destructor->root_tok, &root_offset))
      cprime_internal_error("virtual destructor root is missing");
    loc = (loc - PTR_SIZE) & -PTR_SIZE;
    allocation_slot = loc;
    vset(&char_pointer_type, VT_LOCAL | VT_LVAL, allocation_slot);
    vset(&pointer, VT_LOCAL | VT_LVAL, pointer_slot);
    vtop->type = char_pointer_type;
    if (root_offset) { vpushi(root_offset); gen_op('+'); }
    vset(&pointer, VT_LOCAL | VT_LVAL, pointer_slot);
    vtop->type = char_pointer_type;
    /* Dereference the saved pointer before dereferencing the vfptr field.
       A zero field offset performs no pointer arithmetic to force this load. */
    gv(RC_INT);
    if (field_offset) { vpushi(field_offset); gen_op('+'); }
    vtop->r |= VT_LVAL; gv(RC_INT);
    vpushi(PTR_SIZE); gen_op('-');
    vtop->type.t = VT_PTRDIFF_T; vtop->type.ref = NULL;
    vtop->r |= VT_LVAL;
    gen_op('+'); vstore(); vpop();
  }
  if (destructor) {
    /* Deallocation remains required when an explicitly throwing destructor
       exits exceptionally. The saved complete-object pointer also handles
       deletion through a secondary base subobject. */
    object = sym_push2(&local_stack, SYM_FIELD, 0, allocation_slot);
    object->type = cpp_eh_void_type; object->r = VT_LLOCAL | VT_LVAL;
    cleanup_depth = cur_scope->cl.n;
    cleanup = cpp_eh_register_local_cleanup(object, "free");
    cleanup->a.cleanup_exception_only = 1;
    if (array_delete) {
      CType count_type = { VT_LLONG | VT_UNSIGNED, NULL };
      Sym *array_cleanup;
      int context, count_slot, loop, done, size, alignment;
      size = type_size(&object_type, &alignment);
      loc = (loc - 8) & -8; count_slot = loc;
      vset(&count_type, VT_LOCAL | VT_LVAL, count_slot);
      vset(&char_pointer_type, VT_LOCAL | VT_LVAL, pointer_slot);
      vpushi(PTR_SIZE); gen_op('-');
      vtop->type = count_type; vtop->r |= VT_LVAL;
      vstore(); vpop();
      array_cleanup = cpp_eh_register_array_cleanup(pointer_slot, count_slot,
          size, destructor, &context);
      array_cleanup->a.cleanup_exception_only = 1;
      count_slot = context + offsetof(CpcEhArray, count);
      loop = ind;
      vset(&count_type, VT_LOCAL | VT_LVAL, count_slot);
      done = gvtst(1, 0);
      vset(&count_type, VT_LOCAL | VT_LVAL, count_slot);
      vdup(); vpushi(1); gen_op('-'); vstore(); vpop();
      vpushsym(&destructor->type, destructor);
      vset(&char_pointer_type, VT_LOCAL | VT_LVAL, pointer_slot);
      vset(&count_type, VT_LOCAL | VT_LVAL, count_slot);
      vpushi(size); gen_op('*'); gen_op('+');
      vtop->type = object_type;
      mk_pointer(&vtop->type);
      gfunc_param_typed(destructor->type.ref, destructor->type.ref->next);
      gfunc_call(1); drop_leaked_call_target(destructor);
      gjmp_addr(loop); gsym(done);
    } else {
    vset(&pointer, VT_LOCAL | VT_LVAL, pointer_slot);
    if (virtual_destructor)
      push_virtual_call_target(get_struct_type_name_tok(&object_type),
                               virtual_destructor, destructor);
    else
      vpushsym(&destructor->type, destructor);
    vswap();
    gfunc_param_typed(destructor->type.ref, destructor->type.ref->next);
    gfunc_call(1);
    drop_leaked_call_target(destructor);
    }
    cur_scope->cl.s = cleanup->next;
    cur_scope->cl.n = cleanup_depth;
  }
  vpushsym(&free_function->type, free_function);
  vset(&char_pointer_type, VT_LOCAL | VT_LVAL, allocation_slot);
  gfunc_param_typed(free_function->type.ref,
      free_function->type.ref ? free_function->type.ref->next : NULL);
  gfunc_call(1);
  gsym(skip_null);
  vset(&cpp_eh_void_type, VT_CONST, 0);
}

static void cpp_eh_parse_try(void)
{
  CppEhFunction *eh = &cpp_eh_function;
  int start, end, done = 0, saved_nocode = nocode_wanted;
  int context_offset, cleanup_stop, saw_catch_all = 0;
  if (!is_cpp_translation_unit()) cprime_error("try requires C++");
  if (tok != '{') expect("compound statement after try");
  loc = (loc - sizeof(CpcEhCatchContext)) & -PTR_SIZE;
  context_offset = loc;
  cleanup_stop = cpp_eh_cleanup_index(cur_scope->cl.s);
  start = ind;
  block(0);
  end = ind;
  nocode_wanted = saved_nocode;
  done = gjmp(done);
  if (tok != TOK_CATCH) expect("catch after try block");
  while (tok == TOK_CATCH) {
    struct scope handler_scope;
    CppEhCatchEntry *entry;
    CType type, caught_type;
    AttributeDef attr;
    Sym *context_symbol, *variable = NULL, *dtor;
    SValue source, destination;
    int name = 0, size, align, variable_offset, reference = 0;
    int class_copy = 0, boundary_slot = 0;
    int landing = ind, catch_all = 0;
    nocode_wanted = saved_nocode;
    if (saw_catch_all) cprime_error("catch (...) must be the last handler");
    next(); skip('(');
    new_scope(&handler_scope);
    memset(&attr, 0, sizeof attr);
    if (tok == TOK_DOTS) { next(); catch_all = saw_catch_all = 1; }
    else {
      if (!parse_btype(&type, &attr, 0)) expect("exception declaration");
      type_decl(&type, &attr, &name, TYPE_DIRECT | TYPE_ABSTRACT | TYPE_PARAM);
      reference = is_reference_type(&type);
      if (type.t & VT_RVALUE_REFERENCE) cprime_error("exception handler cannot use an rvalue reference");
      caught_type = reference ? *pointed_type(&type) : type;
    }
    skip(')');
    entry = NULL;
    if (!saved_nocode) {
      eh->catches = cprime_realloc(eh->catches, (eh->catch_count + 1) * sizeof(*eh->catches));
      entry = eh->catches + eh->catch_count++;
      memset(entry, 0, sizeof(*entry));
      entry->data.begin_rva = start;
      entry->data.end_rva = end;
      entry->data.landing_rva = landing;
      entry->data.cleanup_stop = cleanup_stop;
      entry->data.context_offset = context_offset;
      if (!catch_all) entry->type = cpp_eh_type_descriptor(&caught_type);
    }
    context_symbol = sym_push(anon_sym++, &char_pointer_type, VT_LOCAL | VT_LVAL, context_offset);
    if (!catch_all && !reference && (caught_type.t & VT_BTYPE) == VT_STRUCT) {
      CType pointer = caught_type;
      class_copy = 1;
      mk_pointer(&pointer);
      size = type_size(&type, &align);
      loc = (loc - size) & -align;
      variable_offset = loc;
      variable = sym_push(name ? name : anon_sym++, &type, VT_LOCAL | VT_LVAL, variable_offset);
      loc = (loc - PTR_SIZE) & -PTR_SIZE;
      boundary_slot = loc;
      cpp_eh_push_helper("__cpc_eh_begin_catch_copy");
      cpp_eh_push_local_address(0);
      gfunc_call(1);
      vset(&char_pointer_type, VT_LOCAL | VT_LVAL, boundary_slot);
      vset(&char_pointer_type, REG_IRET, 0);
      vstore(); vpop();
      vset(&pointer, VT_LOCAL | VT_LVAL, context_offset + offsetof(CpcEhCatchContext, adjusted));
      source = *vtop--;
      vset(&pointer, VT_LOCAL, variable_offset); destination = *vtop--;
      if (!resolve_copy_constructor_func(&caught_type, &caught_type))
        copy_construct_struct_memberwise_from_base_ptr(&caught_type, &destination, &source, 0, 0);
      else
        copy_construct_one_lvalue_pair(&caught_type, &destination, &source, 0, 0);
      cpp_eh_push_helper("__cpc_eh_end_catch_copy");
      vset(&char_pointer_type, VT_LOCAL | VT_LVAL, boundary_slot);
      gfunc_call(1);
    }
    cpp_eh_push_helper("__cpc_eh_begin_catch");
    cpp_eh_push_local_address(context_offset);
    gfunc_call(1);
    cpp_eh_register_local_cleanup(context_symbol, "__cpc_eh_end_catch");
    if (!catch_all && !class_copy) {
      size = type_size(&type, &align);
      loc = (loc - size) & -align;
      variable_offset = loc;
      variable = sym_push(name ? name : anon_sym++, &type, VT_LOCAL | VT_LVAL, variable_offset);
      if (reference) {
        vset(&char_pointer_type, VT_LOCAL | VT_LVAL, variable_offset);
        vset(&char_pointer_type, REG_IRET, 0);
        vstore(); vpop();
      } else {
        CType pointer = caught_type; mk_pointer(&pointer);
        vset(&pointer, REG_IRET, 0); source = *vtop--;
        vset(&pointer, VT_LOCAL, variable_offset); destination = *vtop--;
        copy_construct_one_lvalue_pair(&caught_type, &destination, &source, 0, 0);
      }
    }
    if (variable && !reference) {
      dtor = resolve_autodtor_func(&caught_type);
      if (dtor) {
          Sym *cleanup = sym_push2(&all_cleanups, SYM_FIELD | ++cur_scope->cl.n, 0, 0);
          cleanup->cleanup_sym = variable;
          cleanup->cleanup_func = dtor;
          cleanup->next = cur_scope->cl.s;
          cur_scope->cl.s = cleanup;
      }
      else if ((caught_type.t & VT_BTYPE) == VT_STRUCT)
        register_struct_cleanups(&caught_type, variable);
    }
    if (tok != '{') expect("compound statement after catch");
    block(0);
    prev_scope(&handler_scope, 0);
    done = gjmp(done);
    nocode_wanted = saved_nocode;
  }
  gsym(done);
}
