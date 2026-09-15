struct Value { int numbers[3]; };
struct Number { int value; };
int global;
inline int unused() {
    constexpr int values[] = {4, 9};
    static_assert(values[1] == 9);
    static_assert(sizeof(values[100]) == sizeof(int));
    return 0;
}
int main() {
    constexpr Value value{{4, 9}};
    static_assert(value.numbers[0] == 4);
    static_assert(value.numbers[1] == 9);
    static_assert(value.numbers[2] == 0);
    static_assert(sizeof(value.numbers[100]) == sizeof(int));
    constexpr int matrix[2][2] = {{1, 2}, {3}};
    static_assert(matrix[0][1] == 2);
    static_assert(matrix[1][0] == 3);
    static_assert(matrix[1][1] == 0);
    constexpr Number objects[2] = {{5}, {8}};
    static_assert(objects[0].value == 5);
    static_assert(objects[1].value == 8);
    constexpr double fractions[] = {2.5, 4.5};
    static_assert(fractions[1] == 4.5);
    constexpr int *pointers[] = {&global, nullptr};
    static_assert(pointers[0] == &global);
    static_assert(pointers[1] == nullptr);
    constexpr Value copy = value;
    static_assert(copy.numbers[1] == 9);
    volatile int index = 1;
    return value.numbers[0] != 4 || value.numbers[index] != 9 || value.numbers[2] != 0
        || matrix[index][0] != 3 || objects[index].value != 8
        || fractions[index] != 4.5 || pointers[index] != nullptr
        || copy.numbers[index] != 9;
}
