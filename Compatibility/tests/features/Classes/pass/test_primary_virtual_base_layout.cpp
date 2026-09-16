// EXPECT_COMPILE_ARGS: -Werror
int construction_order;
struct Plain {
  int marker;
  Plain(): marker(3) { construction_order = construction_order * 10 + 1; }
  int get() const { return marker; }
  void set(int value) { marker = value; }
  template<class T> void set_typed(T value) { marker = value; }
};
struct Primary {
  int value;
  Primary(): value(5) { construction_order = construction_order * 10 + 2; }
  virtual int read() { return value; }
};
struct Derived : Plain, Primary {
  int extra;
  Derived(): extra(7) {}
  int read() override { return value + extra; }
  virtual int added() { return marker + extra; }
};
struct More : Derived {
  int added() override { return extra * 2; }
  virtual int last() { return marker * value; }
};
struct Sibling : Primary {
  virtual int different() { return value * 3; }
};
struct Secondary {
  int second;
  Secondary(): second(11) {}
  virtual int other() { return second; }
};
struct Multiple : Derived, Secondary {
  int other() override { return marker + second; }
  virtual int combined() { return value + second; }
};
int primary_call(Primary* p) { return p->read(); }
int derived_call(Derived* p) { return p->added(); }
int secondary_call(Secondary* p) { return p->other(); }
int main() {
  Derived object;
  if (construction_order != 12) return 1;
  if ((char*)&object.value - (char*)&object != sizeof(void*)) return 2;
  if ((char*)(Primary*)&object != (char*)&object) return 3;
  if ((char*)(Plain*)&object - (char*)&object != sizeof(Primary)) return 4;
  if (sizeof(Derived) != sizeof(Primary) + 2 * sizeof(int)) return 5;
  if (primary_call(&object) != 12 || derived_call(&object) != 10) return 6;
  More more;
  Sibling sibling;
  if (sizeof(More) != sizeof(Derived) || sizeof(Sibling) != sizeof(Primary)) return 7;
  if (derived_call(&more) != 14 || more.last() != 15 || sibling.different() != 15) return 8;
  Multiple multiple;
  if (sizeof(Multiple) != sizeof(Derived) + sizeof(Secondary)) return 9;
  if (secondary_call(&multiple) != 14 || multiple.combined() != 16) return 10;
  if ((char*)(Secondary*)&multiple - (char*)&multiple != sizeof(Derived)) return 11;
  if (object.get() != 3) return 12;
  object.set(9);
  if (object.marker != 9) return 13;
  object.set_typed(13);
  if (object.marker != 13) return 14;
  return 0;
}
