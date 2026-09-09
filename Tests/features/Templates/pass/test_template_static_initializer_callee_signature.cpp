// EXPECT_EXIT: 0
template<class T> int outer(T);
int result = outer(3);
template<class T> inline long inner(T value, int increment) {
    return (long)value + increment;
}
template<class T> struct Value { static const int number; };
template<class T> const int Value<T>::number = inner(3.0, 2);
template<class T> int outer(T) { return Value<double>::number; }
int main() { return Value<double>::number != 5; }
