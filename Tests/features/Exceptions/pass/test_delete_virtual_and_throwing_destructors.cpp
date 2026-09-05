static int order;

struct Member { ~Member() { order = order * 10 + 4; } };
struct First { int first; First():first(1) {} virtual ~First() { order = order * 10 + 1; } };
struct Second { int second; Second():second(2) {} virtual ~Second() { order = order * 10 + 2; } };
struct Derived : First, Second {
  Member member;
  ~Derived() { order = order * 10 + 3; }
};
struct Throwing { ~Throwing() noexcept(false) { throw 7; } };

int main() {
  Derived *derived = new Derived;
  Second *second = derived;
  delete second;
  if (order != 3421) return 1;
  First *first = new Derived;
  order = 0;
  delete first;
  if (order != 3421) return 2;
  First *empty = 0;
  delete empty;
  if (order != 3421) return 3;
  try { delete new Throwing; return 4; }
  catch (int value) { if (value != 7) return 5; }
  return 0;
}
