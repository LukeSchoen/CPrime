// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
int main() {
  bool value = false;
  bool* pointer = &value;
  bool input = true;
  (decltype(*pointer))input = false;
  if (input) return 1;
  (decltype(*pointer))input = true;
  if (!(decltype(*pointer))input) return 2;
  int integer = 7;
  typedef int& Reference;
  (Reference)integer = 9;
  if (&(Reference)integer != &integer) return 3;
  const int& readonly = integer;
  (int&)readonly = 11;
  return integer != 11;
}
