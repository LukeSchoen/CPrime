#ifndef DEFAULTED_ASSIGNMENT_ODR_H
#define DEFAULTED_ASSIGNMENT_ODR_H
struct Assigned {
  int value;
  Assigned() = default;
  Assigned(const Assigned&) = default;
  Assigned(Assigned&&) = default;
  Assigned& operator=(const Assigned&) = default;
  Assigned& operator=(Assigned&&) = default;
};
Assigned assigned_from_other(Assigned value);
#endif
