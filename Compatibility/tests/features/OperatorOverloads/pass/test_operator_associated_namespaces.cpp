// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
namespace first {
struct Base { int value; };
enum class Code { one = 1 };
int operator+(const Base& value, int amount) { return value.value + amount; }
}
namespace second {
struct Derived : first::Base {};
enum class Code { two = 2 };
int operator+(first::Code, Code) { return 12; }
}
namespace holder { template<class T> struct Box { int value; }; }
namespace first {
int operator+(holder::Box<Code> value, int amount) { return value.value + amount + 20; }
}
int operator+(const first::Base&, long) { return 99; }
int main() {
  second::Derived derived;
  derived.value = 4;
  if (derived + 3 != 7) return 1;
  if (first::Code::one + second::Code::two != 12) return 2;
  holder::Box<first::Code> box = {5};
  return box + 2 != 27;
}
