#include "deduced_specialization_link.h"
template<> int special_link::Value::get(int value) const { return bias + value; }
template<> int special_link::calculate(int value) { return value * 2; }
