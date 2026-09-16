struct Padding { int padding; };
struct Calculator {
  int value;
  int add(int n) { return value + n; }
  int get() const { return value; }
  virtual int virtual_get() { return value + 1; }
};
struct Derived : Padding, Calculator {
  int virtual_get() { return value + 2; }
};
int invoke(Calculator *object, int (Calculator::*function)(int), int n) {
  return (object->*function)(n);
}
int main() {
  Calculator calculator;
  calculator.value = 4;
  int (Calculator::*function)(int) = &Calculator::add;
  int Calculator::*field = &Calculator::value;
  int (Calculator::*getter)() const = &Calculator::get;
  if ((calculator.*function)(3) != 7) return 1;
  if (invoke(&calculator, function, 5) != 9) return 2;
  calculator.*field = 11;
  if ((calculator.*getter)() != 11) return 3;
  Derived derived;
  derived.padding = 20;
  derived.value = 30;
  if ((derived.*function)(4) != 34) return 4;
  auto virtual_function = &Calculator::virtual_get;
  if ((calculator.*virtual_function)() != 12) return 5;
  if ((derived.*virtual_function)() != 32) return 6;
  return 0;
}
