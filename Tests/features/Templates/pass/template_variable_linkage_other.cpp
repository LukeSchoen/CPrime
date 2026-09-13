#include "template_variable_linkage.h"

template<int N>
extern const int external_value = N;

template<int N>
const volatile int volatile_value = N;

const int *template_variable_other_value = &external_value<42>;
const volatile int *template_variable_other_volatile_value = &volatile_value<42>;
