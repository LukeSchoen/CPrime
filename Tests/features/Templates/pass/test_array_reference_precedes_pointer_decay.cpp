// EXPECT_EXIT: 0
// An array-reference binding is preferred over array-to-pointer decay when
// both candidates have exact conversion sequences.
template<class T> int select(T const *) { return 1; }
template<class T, __SIZE_TYPE__ N> int select(T const (&)[N]) { return 0; }

int concrete(int const *) { return 1; }
template<class T, __SIZE_TYPE__ N> int concrete(T const (&)[N]) { return 0; }

int main()
{
  int values[4] = {};
  if (select(values) != 0) return 1;
  return concrete(values) != 0;
}
