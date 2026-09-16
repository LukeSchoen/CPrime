struct Tracked {
  static int alive;
  static int copies;
  const Tracked *self;
  int value;
  Tracked(int number) : self(this), value(number) { ++alive; }
  Tracked(const Tracked &other) : self(this), value(other.value) {
    ++alive;
    ++copies;
  }
  ~Tracked() { --alive; }
};
int Tracked::alive = 0;
int Tracked::copies = 0;

Tracked makeTracked(int value) {
  Tracked local(value);
  return local;
}

struct Small {
  int selected;
  Small() : selected(0) {}
  Small(const Small &) : selected(1) {}
  Small(Small &&) : selected(2) {}
};

Small makeSmall(const Small &source) {
  Small local = source;
  return local;
}

int main() {
  {
    Tracked original(11);
    {
      auto closure = [original]() {
        return original.self == &original && original.value == 11;
      };
      if (!closure() || Tracked::alive != 2 || Tracked::copies != 1) return 1;
      {
        auto second = closure;
        if (!second() || Tracked::alive != 3 || Tracked::copies != 2) return 2;
      }
      if (Tracked::alive != 2) return 3;
    }
    if (Tracked::alive != 1) return 4;
  }
  if (Tracked::alive != 0) return 5;
  {
    Tracked direct = Tracked(7);
    if (direct.self != &direct || Tracked::alive != 1) return 6;
    Tracked copied = Tracked(direct);
    if (copied.self != &copied || Tracked::alive != 2
        || Tracked::copies != 3) return 7;
  }
  if (Tracked::alive != 0) return 8;
  {
    Tracked returned = makeTracked(19);
    if (returned.self != &returned || returned.value != 19
        || Tracked::alive != 1) return 9;
  }
  if (Tracked::alive != 0) return 10;
  Small small;
  Small returned = makeSmall(small);
  // Named return value optimization is optional: either retain the local's
  // copy-construction state, or move that local into the result.
  return returned.selected != 1 && returned.selected != 2;
}
