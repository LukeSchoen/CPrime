int global;
struct Number { int value; };
struct Value {
    int numbers[2][2];
    Number objects[2];
    double fractions[2];
    int *pointers[2];
};
constexpr Value source{{{4, 9}, {7}}, {{3}, {8}}, {2.5, 4.5}, {&global}};
struct Mutable { mutable int numbers[2]; };
constexpr Mutable mutable_source{{1, 2}};
int main() {
    constexpr Value copy = source;
    static_assert(copy.numbers[0][1] == 9);
    static_assert(copy.numbers[1][0] == 7);
    static_assert(copy.numbers[1][1] == 0);
    static_assert(copy.objects[1].value == 8);
    static_assert(copy.fractions[1] == 4.5);
    static_assert(copy.pointers[0] == &global);
    static_assert(copy.pointers[1] == nullptr);
    volatile int index = 1;
    mutable_source.numbers[index] = 6;
    Mutable runtime_copy = mutable_source;
    return copy.numbers[index][0] != 7 || copy.objects[index].value != 8
        || copy.fractions[index] != 4.5 || copy.pointers[index] != nullptr
        || runtime_copy.numbers[index] != 6;
}
