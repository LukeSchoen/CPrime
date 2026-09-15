struct Value { int number; };
constexpr Value source{9};
int global;
struct Outer { int prefix; Value member; int *pointer; };
constexpr Outer outer{3, {7}, &global};
constexpr Outer global_copy = outer;
static_assert(global_copy.prefix == 3);
static_assert(global_copy.member.number == 7);
static_assert(global_copy.pointer == &global);
struct Empty {};
const Empty empty{};
constexpr Empty empty_copy = empty;
int main() {
    constexpr Value copy = source;
    static_assert(copy.number == 9);
    constexpr Outer nested = outer;
    static_assert(nested.prefix == 3);
    static_assert(nested.member.number == 7);
    static_assert(nested.pointer == &global);
    return copy.number != 9;
}
