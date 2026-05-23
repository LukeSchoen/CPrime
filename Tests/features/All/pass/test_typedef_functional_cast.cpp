// EXPECT_EXIT: 0

typedef unsigned int ma_uint32;

int main(void)
{
  ma_uint32 a = ma_uint32(123);
  ma_uint32 b = ma_uint32();
  if (a != 123)
    return 1;
  if (b != 0)
    return 2;
  return 0;
}

