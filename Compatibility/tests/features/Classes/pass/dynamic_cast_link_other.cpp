#include "dynamic_cast_link.h"
RttiBase *make_rtti_object(bool derived)
{
  static RttiBase base;
  static RttiDerived child;
  return derived ? static_cast<RttiBase *>(&child) : &base;
}
