void asm_named_target() asm("test_asm_named_target");

int call_through_local_asm_name()
{
  void asm_named_target() asm("test_asm_named_target");
  asm_named_target();
  return 7;
}

void asm_named_target()
{
}

int main()
{
  return call_through_local_asm_name() == 7 ? 0 : 1;
}
