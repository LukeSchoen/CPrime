struct Base { int value; explicit Base(int n) : value(n) {} };
struct Prefix { long long prefix; Prefix() : prefix(123) {} };
struct Derived : Prefix, public Base { Derived() : Base(59) {} };
class Private : Base { public: Private() : Base(67) {} };
struct Left : Base { Left() : Base(71) {} };
struct Right : Base { Right() : Base(73) {} };
struct Ambiguous : Left, Right {};
int main() {
  Derived object;
  try { throw object; }
  catch (const Base &base) { if (base.value != 59) return 1; }
  catch (...) { return 2; }
  try { throw &object; }
  catch (const Base *base) { if (!base || base->value != 59) return 3; }
  catch (...) { return 4; }
  Private hidden;
  try { throw hidden; }
  catch (const Base &) { return 5; }
  catch (const Private &) {}
  Ambiguous ambiguous;
  try { throw ambiguous; }
  catch (const Base &) { return 6; }
  catch (const Ambiguous &) {}
  return 0;
}
