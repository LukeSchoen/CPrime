// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
namespace aliases {
template<class T> using Pointer = T*;
template<class T> using Reference = T&;
template<class T, int N> using Array = T[N];
template<class T> struct Box { T value; };
template<class T> using Wrapped = Box<T>;
}
template<class T> int pointer(aliases::Pointer<T> p) { return sizeof(T) + *p; }
template<class T> int reference(aliases::Reference<T> p) { return sizeof(T) + p; }
template<class T, int N> int array(aliases::Array<T, N>& p) { return N + sizeof(T) + p[1]; }
template<class T> int wrapped(aliases::Wrapped<T> p) { return sizeof(T) + p.value; }
int main() {
  int value = 9;
  int values[2] = {1, 3};
  aliases::Box<int> box = {5};
  return pointer(&value) != 13 || reference(value) != 13
      || array(values) != 9 || wrapped(box) != 9;
}
