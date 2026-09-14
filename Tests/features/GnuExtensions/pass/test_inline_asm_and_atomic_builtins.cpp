// GNU asm and atomic/sync builtin semantics.

struct MemoryOperand
{
  MemoryOperand() : s(0) {}
  ~MemoryOperand() {}
  int s;
};

static void asm_empty_forms(int i)
{
  __asm__("" : );
  __asm__("" : "+g" (i));
  __asm__("" :: );
  __asm__("" :: "g" (i));
  __asm__("" : : );
  __asm__("" : : "g" (i));
  __asm__("" ::: );
  __asm__("" ::: "memory");
  __asm__("" :: : );
  __asm__("" :: "g" (i) : );
  __asm__("" : : : );
  __asm__("" : "+g" (i) : "g" (i) : "memory");
}

static void asm_struct_memory_operand(MemoryOperand &s)
{
  __asm volatile ("" : "+m,r" (s) : : "memory");
}

typedef int v2si __attribute__((vector_size(4 * sizeof(int))));

static const v2si vector_operand = {0x201, 0};
static const int scalar_operand = 0x201;

static void asm_constant_operands()
{
  __asm volatile ("" : : "m" (vector_operand));
  __asm volatile ("" : : "m" (scalar_operand));
}

static int atomic_always_lock_free_probe(int *p)
{
  return __atomic_always_lock_free(4, ({ __asm (""); p; })) ? 0 : 1;
}

struct SyncTarget
{
  void release() volatile
  {
    __sync_lock_release(&t);
    __sync_synchronize();
  }

  bool t;
};

int main()
{
  int local = 3;
  asm_empty_forms(local);

  MemoryOperand target;
  asm_struct_memory_operand(target);
  asm_constant_operands();

  if (int code = atomic_always_lock_free_probe(&local)) { return code; }

  SyncTarget sync = {false};
  sync.release();
  return sync.t ? 1 : 0;
}
