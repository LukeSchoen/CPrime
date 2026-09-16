// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
template<class... T> struct Tuple {};
int identify(Tuple<int*, double*>&) { return 2; }
int identify(Tuple<int, int>&) { return 1; }
int identify(Tuple<>&) { return 3; }
namespace aliases {
template<class T> using Pointer = T*;
template<class... T> using Pointers = Tuple<T*...>;
template<class T> using Value = typename T::type;
}
struct Has { using type = int; };
template<class T> struct Holder {
  using type = aliases::Pointer<T>;
  type value;
};
int main() {
  aliases::Pointers<int, double> pointers;
  aliases::Pointers<> empty;
  aliases::Value<Has> value = 9;
  Holder<int> holder = {&value};
  aliases::Pointer<aliases::Pointer<int>> nested = &holder.value;
  return identify(pointers) != 2 || identify(empty) != 3 || **nested != 9;
}
