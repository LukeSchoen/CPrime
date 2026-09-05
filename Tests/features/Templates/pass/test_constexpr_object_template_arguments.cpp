// EXPECT_EXIT: 0
constexpr int count = 9;
namespace constants {
template<class T> struct Trait { static constexpr bool value = false; };
}
template<int N> struct Size { int value() { return N; } };
template<bool B> struct Flag { int value() { return B ? 1 : 2; } };
template<class T> int evaluate() {
    constexpr int extra = 4;
    Size<count + extra> size;
    Flag<constants::Trait<T>::value> flag;
    return size.value() + flag.value();
}
int main() { return evaluate<int>() == 15 ? 0 : 1; }
