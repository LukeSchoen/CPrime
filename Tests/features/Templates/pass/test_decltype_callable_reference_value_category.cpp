// EXPECT_EXIT: 0
template<class T> struct Kind { int value() { return 1; } };
template<class T> struct Kind<T &> { int value() { return 2; } };
template<class T> struct Kind<T &&> { int value() { return 3; } };
int number = 7;
int &get_number(int unused) { return number; }
struct Value { int number; };
Value make_value(int unused) { Value value = {19}; return value; }
template<class Function> int inspect(Function function) {
    Kind<decltype(function(0))> reference;
    return reference.value();
}
template<class Function> int inspect_value(Function function) {
    Kind<decltype(function(0))> value;
    return value.value();
}
int main() {
    Value object;
    Kind<decltype(object.number)> member;
    Kind<decltype((object.number))> parenthesized;
    Kind<decltype(static_cast<int &&>(number))> moved;
    Kind<decltype(true ? number : object.number)> conditional;
    return inspect(get_number) == 2 && inspect_value(make_value) == 1
        && member.value() == 1
        && parenthesized.value() == 2 && moved.value() == 3
        && conditional.value() == 2 ? 0 : 1;
}
