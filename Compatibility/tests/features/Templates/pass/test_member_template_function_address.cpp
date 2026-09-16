// EXPECT_EXIT: 0
template<class T> struct Value {
  T number;
  template<class U> int subtract(U value, T other) { return value - other; }
  template<class U> int read(U value) {
    int (Value::*function)(U, T);
    function = &Value<T>::template subtract<U>;
    return (this->*function)(value, number);
  }
};
int main() { Value<long> value; value.number = 5; return value.read(12) != 7; }
