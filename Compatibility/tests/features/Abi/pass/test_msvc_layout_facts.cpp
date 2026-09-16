// EXPECT_COMPILE_ARGS: -std=c++17
// Internal replacement for the external msvc_layout ABI gate. Pins the
// Microsoft x64 record layout and dispatch that the Clang-compiled provider
// used to observe: sizes, base offsets, virtual dispatch through each base,
// qualified calls, member pointers and record returns.
#include <stddef.h>

struct LayoutResult { long long first, second; };

struct LayoutPlain {
  int marker;
  LayoutPlain() : marker(3) {}
};

struct LayoutPrimary {
  int value;
  LayoutPrimary() : value(5) {}
  virtual int read() { return value; }
};

struct LayoutDerived : LayoutPlain, LayoutPrimary {
  int extra;
  LayoutDerived() : extra(7) {}
  int read() override { return value + extra; }
  virtual int added() { return marker + extra; }
};

struct LayoutSecondary {
  int second;
  LayoutSecondary() : second(11) {}
  virtual int other() { return second; }
  virtual LayoutResult result(int v) { LayoutResult r = {second, v}; return r; }
};

struct LayoutMultiple : LayoutDerived, LayoutSecondary {
  int other() override { return marker + second; }
  virtual int combined() { return value + second; }
  LayoutResult result(int v) override {
    LayoutResult r = {marker + second, v + extra};
    return r;
  }
};

static_assert(sizeof(LayoutDerived) == 24, "derived record size");
static_assert(sizeof(LayoutMultiple) == 40, "multiple-inheritance record size");

int main() {
  LayoutMultiple object;
  if ((char *)(LayoutPrimary *)&object != (char *)&object) return 1;
  if ((char *)&object.value - (char *)&object != 8) return 2;
  if ((char *)&object.marker - (char *)&object != 16) return 3;
  if ((char *)&object.extra - (char *)&object != 20) return 4;
  if ((char *)(LayoutSecondary *)&object - (char *)&object != 24) return 5;
  if (object.marker != 3 || object.value != 5 || object.extra != 7
      || object.second != 11) return 6;

  LayoutPrimary *primary = &object;
  LayoutDerived *derived = &object;
  LayoutSecondary *secondary = &object;
  if (primary->read() != 12 || derived->added() != 10) return 7;
  if (secondary->other() != 14 || object.combined() != 16) return 8;
  if (object.LayoutMultiple::other() != 14) return 9;

  int (LayoutDerived::*member)() = &LayoutDerived::added;
  if ((object.*member)() != 10) return 10;

  LayoutResult virtual_result = secondary->result(4);
  LayoutResult direct_result = object.LayoutMultiple::result(5);
  if (virtual_result.first != 14 || virtual_result.second != 11) return 11;
  if (direct_result.first != 14 || direct_result.second != 12) return 12;
  return 0;
}
