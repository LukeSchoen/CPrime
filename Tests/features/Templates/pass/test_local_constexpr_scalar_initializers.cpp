constexpr int integer() { return 7; }
constexpr double floating() { return 2.5; }
int runtime_value() { return 9; }
int global;
int main() {
    constexpr int value = integer();
    constexpr double fraction = floating();
    constexpr int *pointer = &global;
    const int dynamic = runtime_value();
    return value != 7 || fraction != 2.5 || pointer != &global || dynamic != 9;
}
