// EXPECT_EXIT: 0
template<typename T> int pick(T * const &p)
{
  (void)p;
  return 1;
}

template<typename T> int pick(T *&p)
{
  (void)p;
  return 2;
}

int main()
{
  int value = 0;
  int *p = &value;
  int * const cp = &value;
  if (pick(p) != 2)
    return 1;
  if (pick(cp) != 1)
    return 2;
  return 0;
}
