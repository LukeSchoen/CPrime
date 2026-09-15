struct Value { int number; };
static_assert(Value{4}.number == 4);
static_assert(Value{9}.number == 9);
struct Outer { Value inner; int numbers[2]; double fraction; };
static_assert(Outer{{7}, {3, 8}, 2.5}.inner.number == 7);
static_assert(Outer{{7}, {3, 8}, 2.5}.numbers[1] == 8);
static_assert(Outer{}.numbers[1] == 0);
static_assert(Outer{{7}, {3, 8}, 2.5}.fraction == 2.5);
int main() {}
