struct First {
  int first;
  int add_first(int n) { return first + n; }
};

struct Second {
  int second;
  int add_second(int n) { return second + n; }
};

struct Derived : First, Second {};

int main() {
  Derived object;
  object.first = 3;
  object.second = 7;
  int (Derived::*first)(int) = &Derived::add_first;
  int (Derived::*second)(int) = &Derived::add_second;
  if ((object.*first)(4) != 7) return 1;
  if ((object.*second)(5) != 12) return 2;
  return 0;
}
