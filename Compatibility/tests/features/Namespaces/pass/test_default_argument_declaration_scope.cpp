// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
int global_bias = 2;
int global_default(int input = global_bias) { return input; }
int lambda_parameter;
int lambda_default(int input = [](int lambda_parameter) {
  return lambda_parameter;
}(3)) { return input; }
namespace Settings {
  enum Order { selected = 7 };
  int bias = 10;
  int read_bias() { return bias; }
  int ordinary(int input = selected + read_bias()) { return input; }
  template<class T> struct Box {
    T read(T input = selected + read_bias()) { return input; }
  };
}
namespace Caller {
  enum Order { selected = 99 };
  int read_bias() { return 100; }
  int invoke() {
    int global_bias = 55;
    int selected = 66;
    Settings::Box<int> box;
    if (box.read() != 17 || Settings::ordinary() != 17) return 1;
    if (global_default() != 2) return 2;
    if (lambda_default() != 3) return 4;
    Settings::bias = 20;
    if (box.read() != 27 || Settings::ordinary() != 27) return 3;
    return 0;
  }
}
int main() { return Caller::invoke(); }
