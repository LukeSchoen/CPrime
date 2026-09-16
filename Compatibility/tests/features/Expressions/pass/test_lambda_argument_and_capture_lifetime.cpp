template<class F> int invoke(F function, int value) { return function(value); }
struct Tracked {
  static int copies;
  int value;
  Tracked(int v) : value(v) {}
  Tracked(const Tracked &other) : value(other.value) { ++copies; }
};
int Tracked::copies = 0;
int main() {
  int amount = 7;
  if (invoke([&](int x) { return amount + x; }, 3) != 10) return 1;
  Tracked object(11);
  auto closure = [object]() { return object.value; };
  object.value = 23;
  if (closure() != 11 || Tracked::copies < 1) return 2;
  int originalCopies = Tracked::copies;
  auto shadow = [=]() { int object = 31; return object; };
  if (shadow() != 31 || Tracked::copies != originalCopies) return 3;
  return 0;
}
