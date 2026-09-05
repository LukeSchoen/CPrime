// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
template<class T> struct Reference {
  T *pointer;
  explicit Reference(T &value) : pointer(&value) {}
  operator T&() const { return *pointer; }
};
int read(const int &value) { return value; }
void increment(int &value) { ++value; }
int select(int &value) { return ++value; }
int select(const int &value) { return value + 100; }
struct Object { int value; };
struct Scalar { operator long() const { return 23; } };
int object_value(const Object &value) { return value.value; }
struct OutOfLine {
  int *pointer;
  explicit OutOfLine(int &value) : pointer(&value) {}
  operator int&() const;
};
OutOfLine::operator int&() const { return *pointer; }
struct VirtualReference {
  virtual operator int&() = 0;
};
struct ConcreteReference : VirtualReference {
  int *pointer;
  explicit ConcreteReference(int &value) : pointer(&value) {}
  operator int&() { return *pointer; }
};
int main() {
  int value = 7;
  const Reference<int> reference(value);
  increment(reference);
  if (value != 8 || read(reference) != 8 || select(reference) != 9) return 1;
  int scalar = reference;
  if (scalar != 9) return 2;
  Scalar converted_value;
  scalar = converted_value;
  if (scalar != 23 || read(converted_value) != 23) return 5;
  Object object = {17};
  Reference<Object> wrapped_object(object);
  if (object_value(wrapped_object) != 17) return 3;
  OutOfLine outside(value);
  increment(outside);
  if (value != 10) return 4;
  ConcreteReference concrete(value);
  VirtualReference &base = concrete;
  increment(base);
  return value != 11;
}
