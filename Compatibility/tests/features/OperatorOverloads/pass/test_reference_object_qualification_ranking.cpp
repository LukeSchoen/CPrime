// EXPECT_EXIT: 0
int select(int *&) { return 1; }
int select(int *const &) { return 2; }
int select(int *volatile &) { return 3; }
int select(int *const volatile &) { return 4; }
int *make_pointer(int *value) { return value; }
struct Value {};
int select(Value &) { return 5; }
int select(const Value &) { return 6; }
struct Member {
  int choose(int *&) { return 7; }
  int choose(int *const &) { return 8; }
};
int main() {
  int item = 0;
  int *pointer = &item;
  int *const immutable = pointer;
  int *volatile observable = pointer;
  int *const volatile both = pointer;
  if (select(pointer) != 1 || select(immutable) != 2) return 1;
  if (select(observable) != 3 || select(both) != 4) return 2;
  if (select(make_pointer(pointer)) != 2) return 5;
  Value value;
  const Value constant;
  if (select(value) != 5 || select(constant) != 6) return 3;
  Member member;
  if (member.choose(pointer) != 7 || member.choose(immutable) != 8) return 4;
  return 0;
}
