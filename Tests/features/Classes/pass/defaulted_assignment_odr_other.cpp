#include "defaulted_assignment_odr.h"
Assigned assigned_from_other(Assigned value) {
  Assigned copy, moved;
  copy = value;
  moved = static_cast<Assigned&&>(copy);
  return moved;
}
