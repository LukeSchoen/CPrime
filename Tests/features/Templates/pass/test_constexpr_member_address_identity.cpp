struct Value { int prefix; int number; };
constexpr Value value{4, 9};
constexpr const int *pointer = &value.number;
constexpr const int *empty = nullptr;
static_assert(pointer == &value.number);
static_assert(*pointer == 9);
static_assert(empty == nullptr);
int main() { return *pointer != 9; }
