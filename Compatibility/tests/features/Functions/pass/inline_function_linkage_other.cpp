#include "inline_function_linkage.h"
InlineReferenceFunction other_reference_address() { return &InlineApi::value; }
InlineValueFunction other_external_address() { return &InlineApi::explicit_external; }
InlineValueFunction other_internal_address() { return &InlineApi::internal; }
InlineValueFunction other_c_address() { return &c_inline_value; }
int other_call() { return InlineApi::value(4) + InlineApi::explicit_external(5) + c_inline_value(6); }
