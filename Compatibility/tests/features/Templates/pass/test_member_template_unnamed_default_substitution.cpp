// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
template<class... T> using Void = void;
struct Has { using type = int; int value; };
struct Reader {
  int value;
  template<class T, class = Void<typename T::type>>
  Reader(const T& input) : value(input.value) {}
  template<class T, class = Void<typename T::type>>
  int read(const T& input) { return input.value + value; }
};
int main() {
  Has value = {9};
  Reader reader(value);
  return reader.value != 9 || reader.read(value) != 18;
}
