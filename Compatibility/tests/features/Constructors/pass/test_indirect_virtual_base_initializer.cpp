// A virtual base inherited through another base has no base field of its own in
// the most-derived class, so its mem-initializer used to be dropped and the
// base default-initialized instead.  The most-derived constructor owns that
// subobject: the initializer it writes applies, the intermediate base's own
// initializer for the same base is ignored at run time, and the subobject is
// constructed exactly once.
int constructions;

struct Measure {
  int value;
  Measure(int n) : value(n) { ++constructions; }
  Measure() : value(-1) { ++constructions; }
};

struct Middle : virtual Measure {
  int middle;
  Middle() : Measure(1) { middle = 11; }
};

struct Derived : Middle {
  int derived;
  Derived() : Measure(2) { derived = 22; }
};

struct Deeper : Derived {
  int deeper;
  Deeper() : Measure(3) { deeper = 33; }
};

struct Second : virtual Measure {
  int second;
  Second() : Measure(5) { second = 55; }
};

struct Both : Middle, Second {
  int both;
  Both() : Measure(4), Second() { both = 44; }
};

// No user-declared constructor: the intermediate base has no constructor
// symbol to call, so its implicit construction must leave the virtual base
// subobject to the complete-object constructor too.
struct Quiet : virtual Measure {
  int quiet;
};

struct ThroughQuiet : Quiet {
  int through;
  ThroughQuiet() : Measure(6) { through = 66; }
};

int main() {
  constructions = 0;
  Derived d;
  if (d.value != 2 || d.middle != 11 || d.derived != 22) return 1;
  if (constructions != 1) return 2;

  constructions = 0;
  Deeper e;
  if (e.value != 3 || e.middle != 11 || e.derived != 22 || e.deeper != 33)
    return 3;
  if (constructions != 1) return 4;

  constructions = 0;
  Middle m;
  if (m.value != 1 || m.middle != 11) return 5;
  if (constructions != 1) return 6;

  constructions = 0;
  Both b;
  if (b.value != 4 || b.middle != 11 || b.second != 55 || b.both != 44)
    return 7;
  if (constructions != 1) return 8;

  constructions = 0;
  ThroughQuiet q;
  if (q.value != 6 || q.through != 66) return 9;
  if (constructions != 1) return 10;
  return 0;
}
