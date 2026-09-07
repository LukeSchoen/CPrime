#include "dynamic_cast_link.h"
int main()
{
  if (dynamic_cast<RttiDerived *>(make_rtti_object(false))) return 1;
  RttiDerived *child = dynamic_cast<RttiDerived *>(make_rtti_object(true));
  if (!child || child->value() != 2) return 2;
  return 0;
}
