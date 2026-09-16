struct Value { int number; };
int global;
struct Mixed { double number; int *pointer; };
int main() {
    constexpr Value first{4}, second{9};
    static_assert(first.number == 4);
    static_assert(second.number == 9);
    constexpr Mixed mixed{2.5, &global};
    static_assert(mixed.number == 2.5);
    static_assert(mixed.pointer == &global);
    return first.number != 4 || second.number != 9;
}
