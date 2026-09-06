#include "layout.h"
#include <new>
extern "C" int native_layout_check(LayoutMultiple* object) {
  if (sizeof(LayoutMultiple) != 40 || sizeof(LayoutDerived) != 24) return 1;
  if ((char*)(LayoutPrimary*)object != (char*)object) return 2;
  if ((char*)&object->value - (char*)object != 8) return 3;
  if ((char*)&object->marker - (char*)object != 16) return 4;
  if ((char*)&object->extra - (char*)object != 20) return 5;
  if ((char*)(LayoutSecondary*)object - (char*)object != 24) return 6;
  return object->marker != 3 || object->value != 5 || object->extra != 7
       || object->second != 11;
}
extern "C" void native_layout_construct(LayoutMultiple* place) {
  new(place) LayoutMultiple;
}
extern "C" int native_primary_call(LayoutPrimary* object) { return object->read(); }
extern "C" int native_added_call(LayoutDerived* object) { return object->added(); }
extern "C" int native_secondary_call(LayoutSecondary* object) { return object->other(); }
extern "C" int native_combined_call(LayoutMultiple* object) { return object->combined(); }
extern "C" int native_qualified_call(LayoutMultiple* object) {
  return object->LayoutMultiple::other();
}
int LayoutNativeFinal::other() { return marker + second + 1; }
LayoutResult LayoutNativeFinal::result(int v) { LayoutResult r={marker+second+1,v+extra}; return r; }
extern "C" int native_secondary_result(LayoutSecondary* object) {
  LayoutResult r=object->result(4);
  return r.first==14 && r.second==11;
}
