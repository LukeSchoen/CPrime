#include "template_variable_linkage.h"

template<int Value>
extern const int external_value = Value;

template<int Value>
const volatile int volatile_value = Value;

const int *template_variable_other_value = &external_value<42>;
const volatile int *template_variable_other_volatile_value = &volatile_value<42>;
