template <int>
static int reuse_readwrite_output(int value)
{
  __asm__("# %0 %1" : "+r" (value));
  return value;
}

int main()
{
  return reuse_readwrite_output<0>(0) || reuse_readwrite_output<1>(0);
}
