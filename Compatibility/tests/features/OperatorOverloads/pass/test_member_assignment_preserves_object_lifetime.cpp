// EXPECT_EXIT: 0

int copies;
int assignments;
int destructions;

struct Value {
  int value;
  Value(int input = 0) : value(input) {}
  Value(const Value &other) : value(other.value) { ++copies; }
  Value &operator=(const Value &other) {
    value = other.value + 1;
    ++assignments;
    return *this;
  }
  ~Value() { ++destructions; }
};

struct Owner {
  Value member;
  Owner(const Value &input) : member(input) {
    // Assignment in a constructor body still invokes operator=.
    this->member = input;
  }
  void update(const Owner &other) { this->member = other.member; }
};

struct LaterField {
  // This member initializer is lowered after the class is complete.
  LaterField(const Value &input) : member(input) {}
  Value member;
};

int main() {
  {
    Value input(10);
    Owner first(input);
    Owner second(input);
    if (copies != 2 || assignments != 2 || first.member.value != 11)
      return 1;
    first.update(second);
    if (copies != 2 || assignments != 3 || first.member.value != 12)
      return 2;
    LaterField later(input);
    if (copies != 3 || assignments != 3 || later.member.value != 10)
      return 3;
  }
  return destructions == 4 ? 0 : 4;
}
