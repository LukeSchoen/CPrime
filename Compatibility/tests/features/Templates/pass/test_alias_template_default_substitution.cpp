// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
template<class... T> using Void = void;
template<class T, class = Void<typename T::type>>
int choose(int) { return 1; }
template<class T> int choose(long) { return 2; }
template<class T> int reversed(long) { return 4; }
template<class T, class = Void<typename T::type>>
int reversed(int) { return 3; }
struct Has { using type = int; };
template<class T, class = Void<typename T::type>>
int deduced(const T&, int) { return 5; }
template<class T> int deduced(const T&, long) { return 6; }
template<class T> using Member = typename T::type;
template<class T> Member<T> result(int) { return 7; }
template<class T> int result(long) { return 8; }
int main() {
  Has has;
  return choose<Has>(0) != 1 || choose<int>(0) != 2
      || reversed<Has>(0) != 3 || reversed<int>(0) != 4
      || deduced(has, 0) != 5 || deduced(9, 0) != 6
      || result<Has>(0) != 7 || result<int>(0) != 8;
}
