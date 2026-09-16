struct Value { int number; };
struct Mixed { int first; double fraction; int *pointer; int last; };
int main() {
    constexpr Value value{};
    static_assert(value.number == 0);
    constexpr Mixed mixed{7};
    static_assert(mixed.first == 7);
    static_assert(mixed.fraction == 0.0);
    static_assert(mixed.pointer == nullptr);
    static_assert(mixed.last == 0);
    return value.number;
}
