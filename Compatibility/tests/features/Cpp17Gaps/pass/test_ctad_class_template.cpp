// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: ctad_class. Class template argument deduction from a braced
// initializer must find Box<int> without an explicit template argument list.
template<class T> struct Box { T value; };

int main() {
  Box box{1};
  return box.value - 1;
}
