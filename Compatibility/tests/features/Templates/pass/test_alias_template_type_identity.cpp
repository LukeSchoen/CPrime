// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
template<class T> using Identity = T;
template<class T> using Pointer = T*;
template<class T> using Reference = T&;
template<class T, int N> using Array = T[N];
template<class T> using Function = T(T);
namespace aliases { template<class T = int> using Number = T; }
template<class T> struct Box { T value; };
template<class T> using Nested = Box<Pointer<T>>;
int twice(int n) { return 2*n; }
int main() {
  Identity<int> value = 7;
  Pointer<int> pointer = &value;
  Reference<int> reference = value;
  Array<int, 2> array = {2, 3};
  Function<int>* function = &twice;
  aliases::Number<> number = 5;
  Nested<int> nested = {pointer};
  reference = 9;
  return *nested.value != 9 || array[1] != 3 || function(number) != 10;
}
