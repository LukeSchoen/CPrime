// EXPECT_EXIT: 0
template<class A, class B> int select(A, B) { return 1; }
template<class A*, class B> int select(A*, B) { return 2; }

int main()
{
  int value = 0;
  if (select(value, 0) != 1) return 1;
  return select(&value, 0) == 2 ? 0 : 2;
}
