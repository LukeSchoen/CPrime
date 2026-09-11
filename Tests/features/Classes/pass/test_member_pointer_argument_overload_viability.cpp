// A pointer-to-member argument converts to a pointer-to-member parameter when
// the member types agree and the parameter owner is reachable from the
// argument owner.  Ranking already accepted that conversion, but the
// viability predicate did not, so the ranked candidate was discarded and the
// member call fell back to the arity-only lookup.  That fallback selected the
// first same-arity overload (declared before the viable one here) and reported
// a spurious "cannot convert" error.

struct Parent {
  void DoSomething() { }
  int field;
};

struct Child : Parent {
};

struct User {
  int picked;
  void AnyThing(void (Parent::*)(void)) { picked = 1; }
  void Thing(void (Child::*)(void)) { picked = 2; }
  void Thing(int Child::*) { picked = 3; }
};

int main() {
  User user;

  user.picked = 0;
  user.AnyThing(&Child::DoSomething);
  if (user.picked != 1) return 1;

  user.picked = 0;
  user.Thing(&Child::DoSomething);
  if (user.picked != 2) return 2;

  user.picked = 0;
  user.Thing(&Parent::field);
  if (user.picked != 3) return 3;

  return 0;
}
