// EXPECT_COMPILE_ARGS: -Werror
struct Policy {};
struct Wrapper { int value; };

template<class Function, class... Args>
int choose(Policy, Function, Args... args) { return 1; }
template<class Function, class Object, class... Args>
int choose(Policy, Function, Object*, Args... args) { return 2; }

// Reverse declaration order must not change eligibility.
template<class Function, class Object, class... Args>
int reverse(Policy, Function, Object*, Args... args) { return 2; }
template<class Function, class... Args>
int reverse(Policy, Function, Args... args) { return 1; }

template<class T, class... Args>
int consistent(Policy, T*, T*, Args... args) { return 3; }
template<class... Args>
int consistent(Policy, Args... args) { return 4; }

int callable(int value) { return value; }
int main() {
  int value = 0;
  double other = 0;
  Wrapper wrapped = {7};
  if (choose(Policy(), &callable, wrapped) != 1) return 1;
  if (choose(Policy(), &callable, &value) != 2) return 2;
  if (reverse(Policy(), &callable, wrapped, 3) != 1) return 3;
  if (reverse(Policy(), &callable, &value, 3) != 2) return 4;
  if (consistent(Policy(), &value, &other) != 4) return 5;
  if (consistent(Policy(), &value, &value) != 3) return 6;
  return 0;
}
