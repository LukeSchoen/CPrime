// EXPECT_EXIT: 0
auto ordinary(int value) -> decltype(value) { return value + 1; }
template<class T> auto identity(T value) -> decltype(value) { return value; }
template<class T> struct Result { T value; };
template<class T> auto wrap(T value) -> Result<decltype(value)> {
    Result<T> result = {value};
    return result;
}
struct Calculator { int add(int value) { return value + 2; } };
template<class Function, class Object, class... Args>
auto invoke(Function function, Object *object, Args... args)
  -> decltype((object->*function)(args...)) {
    return (object->*function)(args...);
}
int main() {
    if (ordinary(4) != 5) return 1;
    if (identity(7) != 7) return 2;
    if (wrap(9).value != 9) return 3;
    Calculator calculator;
    if (invoke(&Calculator::add, &calculator, 5) != 7) return 4;
    return 0;
}
