// EXPECT_EXIT: 0

struct Values
{
  char b;
  short s;
  int i;
  long long q;
};

int main(void)
{
  struct Values values;
  int local;
  long long ptr_bits;

  values.b = 260;
  values.s = 70000;
  values.i = 123456789;
  values.q = -7;
  local = 42;
  ptr_bits = 0;

  if (values.b != 4)
    return 1;
  if (values.s != 4464)
    return 2;
  if (values.i != 123456789)
    return 3;
  if (values.q != -7)
    return 4;
  if (local != 42)
    return 5;
  if (ptr_bits != 0)
    return 6;
  return 0;
}
