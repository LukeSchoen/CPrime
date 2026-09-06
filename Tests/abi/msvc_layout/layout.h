#pragma once
extern "C" int layout_construction_order;
struct LayoutResult { long long first, second; };
struct LayoutPlain {
  int marker;
  LayoutPlain(): marker(3) {
    layout_construction_order = layout_construction_order * 10 + 1;
  }
};
struct LayoutPrimary {
  int value;
  LayoutPrimary(): value(5) {
    layout_construction_order = layout_construction_order * 10 + 2;
  }
  virtual int read() { return value; }
};
struct LayoutDerived : LayoutPlain, LayoutPrimary {
  int extra;
  LayoutDerived(): extra(7) {}
  int read() override { return value + extra; }
  virtual int added() { return marker + extra; }
};
struct LayoutSecondary {
  int second;
  LayoutSecondary(): second(11) {}
  virtual int other() { return second; }
  virtual LayoutResult result(int v) { LayoutResult r={second,v}; return r; }
};
struct LayoutMultiple : LayoutDerived, LayoutSecondary {
  int other() override { return marker + second; }
  virtual int combined() { return value + second; }
  LayoutResult result(int v) override { LayoutResult r={marker+second,v+extra}; return r; }
};
struct LayoutNativeFinal : LayoutDerived, LayoutSecondary {
  int other() override;
  LayoutResult result(int) override;
};
extern "C" int native_layout_check(LayoutMultiple*);
extern "C" void native_layout_construct(LayoutMultiple*);
extern "C" int native_primary_call(LayoutPrimary*);
extern "C" int native_added_call(LayoutDerived*);
extern "C" int native_secondary_call(LayoutSecondary*);
extern "C" int native_combined_call(LayoutMultiple*);
extern "C" int native_qualified_call(LayoutMultiple*);
extern "C" int native_secondary_result(LayoutSecondary*);
