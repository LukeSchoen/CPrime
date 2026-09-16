int value = 90;
struct Base { int value; };
struct Derived : Base { int read(); };
int Derived::read() {
  int sum = value;
  { auto value = 7; sum += value; }
  sum += value;
  return sum + ::value;
}
int main() { Derived d; d.value = 3; return d.read() != 103; }
