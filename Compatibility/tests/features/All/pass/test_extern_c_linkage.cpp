extern "C" void c_linked_decl(int value);

extern "C"
{
  int c_linked_block_value(void);
}

extern "C" int c_linked_block_value(void)
{
  return 7;
}

int main(void)
{
  return c_linked_block_value() - 7;
}
