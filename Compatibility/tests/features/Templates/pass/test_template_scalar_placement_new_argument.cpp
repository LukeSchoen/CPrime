// EXPECT_EXIT: 0

template<typename T, typename U>
void construct(T *p, U arg)
{
  new(p) T(arg);
}

int main(void)
{
  int value = 0;
  construct(&value, 42);
  return value == 42 ? 0 : 1;
}
