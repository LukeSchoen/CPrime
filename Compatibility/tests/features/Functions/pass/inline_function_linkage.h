#pragma once
namespace InlineApi {
inline int value(const int& input) { return input + 3; }
extern inline int explicit_external(int input) { return input * 2; }
static inline int internal(int input) { return input; }
}
extern "C" inline int c_inline_value(int input) { return input + 1; }
typedef int (*InlineReferenceFunction)(const int&);
typedef int (*InlineValueFunction)(int);
InlineReferenceFunction other_reference_address();
InlineValueFunction other_external_address();
InlineValueFunction other_internal_address();
InlineValueFunction other_c_address();
int other_call();
