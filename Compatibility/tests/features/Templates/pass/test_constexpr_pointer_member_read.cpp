struct Value { int number; };
constexpr Value first{4}, second{9};
constexpr int read(const Value *pointer) { return pointer->number; }
static_assert(read(&first) == 4);
static_assert(read(&second) == 9);
int main() {}
