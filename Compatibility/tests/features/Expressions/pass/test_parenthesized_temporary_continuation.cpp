// EXPECT_EXIT: 0
struct Input {};
namespace Choice { enum Kind { selected }; }
struct Value {
  Value(Input) {}
  Value(Choice::Kind) {}
  int read() { return 3; }
  int operator()() { return 5; }
};
int callback() throw() { return 4; }
int main() {
  int (callback)() throw();
  int result = 0;
  Value(Input()).read();
  Value(Input())() + 1;
  Value(Choice::selected)() + 1;
  result = Value(Input()).read() + Value(Input())();
  return result != 8 || callback() != 4;
}
