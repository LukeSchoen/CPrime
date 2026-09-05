/* Class prvalues remain alive until their enclosing full expression ends.
   Runtime flags distinguish branches that actually completed construction. */
typedef struct CppTemporary {
  Sym *object;
  Sym *cleanup;
  Sym *destructor;
  int flag_offset;
  int storage;
  int address;
  int size;
  int active;
  int extended;
  unsigned used_slot;
} CppTemporary;

static CppTemporary *cpp_temporaries;
static int cpp_temporary_count, cpp_temporary_capacity;
static int cpp_temporary_flag_count;
static unsigned cpp_temporary_reserved_slots;

static void cpp_temp_function_begin(void)
{
  cpp_temporary_count = 0;
  cpp_temporary_flag_count = 0;
  cpp_temporary_reserved_slots = 0;
}

static void cpp_temp_function_end(void)
{
  cpp_temporary_count = 0;
  cpp_temporary_flag_count = 0;
  cpp_temporary_reserved_slots = 0;
}

static int cpp_temp_mark(void) { return cpp_temporary_count; }
static int cpp_temp_has_flags(void) { return cpp_temporary_flag_count != 0; }

static unsigned cpp_temp_used_slots(void)
{
  return cpp_temporary_reserved_slots;
}

static int cpp_temp_cleanup_active_offset(Sym *cleanup)
{
  return cleanup && cleanup->a.cleanup_temporary
      ? (int)cleanup->const_value : 0;
}

static void cpp_temp_store_flag(int address, int value)
{
  vset(&int_type, VT_LOCAL | VT_LVAL, address);
  vpushi(value);
  vstore();
  vpop();
}

static void cpp_temp_emit_flag_initializers(void)
{
  int i;
  for (i = 0; i < cpp_temporary_count; ++i)
    if (cpp_temporaries[i].flag_offset)
      cpp_temp_store_flag(cpp_temporaries[i].flag_offset, 0);
}

static int cpp_temp_guard_cleanup(Sym *cleanup)
{
  int offset = cpp_temp_cleanup_active_offset(cleanup);
  int skip;
  if (!offset) return 0;
  /* Spills must execute on both sides of the activation test. A spill
     emitted only by the destructor call would leave the skipped branch
     referring to an unwritten result slot. */
  if (vtop >= vstack && ((vtop->r & VT_VALMASK) == VT_CMP
      || (vtop->r & VT_VALMASK) == VT_JMP
      || (vtop->r & VT_VALMASK) == VT_JMPI))
    gv(RC_INT);
  save_regs(0);
  vset(&int_type, VT_LOCAL | VT_LVAL, offset);
  skip = gvtst(1, 0);
  cpp_temp_store_flag(offset, 0);
  return skip;
}

static void cpp_register_class_temporary(SValue *value)
{
  CppTemporary *temporary;
  CType type;
  Sym *destructor, *cleanup, *object;
  int storage, i, align;
  if ((value->type.t & VT_BTYPE) != VT_STRUCT
      || !is_cpp_translation_unit()) return;
  value->r |= VT_CXX_PRVALUE;
  value->type.t |= VT_RVALUE_REFERENCE;
  if (nocode_wanted || !cur_scope) return;
  storage = value->r & (VT_VALMASK | VT_LVAL | VT_SYM);
  if (storage != (VT_LOCAL | VT_LVAL)
      && storage != (VT_LLOCAL | VT_LVAL)) return;
  for (i = cpp_temporary_count - 1; i >= 0; --i)
    if (cpp_temporaries[i].active && cpp_temporaries[i].storage == storage
        && cpp_temporaries[i].address == value->c.i) return;
  type = value->type;
  type.t &= ~(VT_STORAGE | VT_RVALUE_REFERENCE);
  destructor = resolve_autodtor_func(&type);
  object = cleanup = NULL;
  if (destructor) {
    object = sym_push(anon_sym++, &type, storage, value->c.i);
    cleanup = sym_push2(&all_cleanups, SYM_FIELD | ++cur_scope->cl.n, 0, 0);
    cleanup->cleanup_sym = object;
    cleanup->cleanup_func = destructor;
    cleanup->next = cur_scope->cl.s;
    cur_scope->cl.s = cleanup;
    loc = (loc - 4) & -4;
    cleanup->a.cleanup_temporary = 1;
    cleanup->const_value = loc;
    ++cpp_temporary_flag_count;
  }
  if (cpp_temporary_count == cpp_temporary_capacity) {
    cpp_temporary_capacity = cpp_temporary_capacity ? 2 * cpp_temporary_capacity : 16;
    cpp_temporaries = cprime_realloc(cpp_temporaries,
        cpp_temporary_capacity * sizeof(*cpp_temporaries));
  }
  temporary = &cpp_temporaries[cpp_temporary_count++];
  memset(temporary, 0, sizeof(*temporary));
  temporary->object = object;
  temporary->cleanup = cleanup;
  temporary->destructor = destructor;
  temporary->flag_offset = cleanup ? loc : 0;
  temporary->storage = storage;
  temporary->address = value->c.i;
  temporary->size = type_size(&type, &align);
  temporary->active = 1;
  i = value->r2 - (VT_CONST + 1);
  if (i >= 0 && i < MAX_TEMP_LOCAL_VARIABLE_NUMBER)
    temporary->used_slot = 1U << i;
  cpp_temporary_reserved_slots |= temporary->used_slot;
  if (temporary->flag_offset) cpp_temp_store_flag(temporary->flag_offset, 1);
}

static CppTemporary *cpp_temp_find_value(SValue *value)
{
  int i, storage = value->r & (VT_VALMASK | VT_LVAL | VT_SYM);
  int64_t address = (int64_t)value->c.i;
  for (i = cpp_temporary_count - 1; i >= 0; --i) {
    CppTemporary *temporary = &cpp_temporaries[i];
    if (temporary->active && !temporary->extended
        && temporary->storage == storage
        && temporary->address <= address
        && address < temporary->address + temporary->size)
      return temporary;
  }
  return NULL;
}

static Sym *cpp_adopt_class_temporary(SValue *value)
{
  CppTemporary *temporary = cpp_temp_find_value(value);
  if (!temporary) return NULL;
  if (temporary->flag_offset) cpp_temp_store_flag(temporary->flag_offset, 0);
  temporary->active = 0;
  cpp_temporary_reserved_slots &= ~temporary->used_slot;
  if (temporary->cleanup) temporary->cleanup->cleanup_func = NULL;
  return temporary->object;
}

static void cpp_temp_extend_lifetime(SValue *value)
{
  CppTemporary *temporary = cpp_temp_find_value(value);
  if (temporary) temporary->extended = 1;
}

static void cpp_temp_end(int mark)
{
  int i;
  if (nocode_wanted) return;
  for (i = cpp_temporary_count - 1; i >= mark; --i) {
    CppTemporary *temporary = &cpp_temporaries[i];
    Sym *object, *destructor, *cleanup;
    int skip;
    if (!temporary->active || temporary->extended) continue;
    if (!temporary->destructor) {
      temporary->active = 0;
      cpp_temporary_reserved_slots &= ~temporary->used_slot;
      continue;
    }
    object = temporary->object;
    destructor = temporary->destructor;
    cleanup = temporary->cleanup;
    /* A pending comparison must become a value before destructor calls
       overwrite flags. gfunc_call then preserves live value registers. */
    if (vtop >= vstack && (vtop->type.t & VT_BTYPE) != VT_VOID
        && (vtop->type.t & VT_BTYPE) != VT_STRUCT)
      gv(RC_TYPE(vtop->type.t));
    skip = cpp_temp_guard_cleanup(cleanup);
    cpp_eh_set_cleanup_state(cleanup->next);
    save_lvalues();
    vpushsym(&destructor->type, destructor);
    vset(&object->type, object->r, object->c);
    mk_pointer(&vtop->type);
    gaddrof();
    gfunc_call(1);
    drop_leaked_call_target(destructor);
    if (vtop >= vstack && (vtop->type.t & VT_BTYPE) == VT_VOID) vpop();
    gsym(skip);
    cpp_eh_restore_cleanup_state();
    temporary = &cpp_temporaries[i];
    temporary->active = 0;
    cpp_temporary_reserved_slots &= ~temporary->used_slot;
    cleanup->cleanup_func = NULL;
  }
}

static void cpp_temp_end_condition(int mark)
{
  if (cpp_temporary_count > mark)
    try_call_cpp_bool_conversion_operator();
  cpp_temp_end(mark);
}
