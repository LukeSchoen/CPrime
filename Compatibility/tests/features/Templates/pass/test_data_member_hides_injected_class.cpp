// EXPECT_EXIT: 0
template<class T> struct Value {
  char Value;
  int read() const { return this->Value; }
  template<class U> int add(U amount) const { return this->Value + amount; }
  int outside() const;
};
template<class T> int Value<T>::outside() const { return this->Value; }
int main() {
  Value<int> value;
  value.Value = 7;
  return value.read() != 7 || value.add(3) != 10 || value.outside() != 7
      || sizeof(value) != 1;
}
