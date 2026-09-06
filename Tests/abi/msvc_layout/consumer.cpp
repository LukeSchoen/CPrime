#include "layout.h"
extern "C" { int layout_construction_order; }
int cpc_primary_call(LayoutPrimary* object) { return object->read(); }
int cpc_added_call(LayoutDerived* object) { return object->added(); }
int cpc_secondary_call(LayoutSecondary* object) { return object->other(); }
int cpc_combined_call(LayoutMultiple* object) { return object->combined(); }
int main() {
  LayoutMultiple object;
  if (layout_construction_order != 12 || native_layout_check(&object)) return 1;
  if (native_primary_call(&object) != 12 || native_added_call(&object) != 10
      || native_secondary_call(&object) != 14 || native_combined_call(&object) != 16)
    return 2;
  if (native_qualified_call(&object) != 14
      || object.LayoutMultiple::other() != 14) return 5;
  if (!native_secondary_result(&object)) return 8;
  layout_construction_order = 0;
  native_layout_construct(&object);
  if (layout_construction_order != 12 || native_layout_check(&object)) return 3;
  if (cpc_primary_call(&object) != 12 || cpc_added_call(&object) != 10
      || cpc_secondary_call(&object) != 14 || cpc_combined_call(&object) != 16)
    return 4;
  LayoutNativeFinal nativeMethod;
  if (cpc_secondary_call(&nativeMethod) != 15
      || nativeMethod.LayoutNativeFinal::other() != 15) return 6;
  int (LayoutNativeFinal::*member)() = &LayoutNativeFinal::other;
  if ((nativeMethod.*member)() != 15) return 7;
  LayoutSecondary* secondary=&nativeMethod;
  LayoutResult virtualResult=secondary->result(4);
  LayoutResult directResult=nativeMethod.LayoutNativeFinal::result(5);
  if (virtualResult.first!=15 || virtualResult.second!=11
      || directResult.first!=15 || directResult.second!=12) return 9;
  return 0;
}
