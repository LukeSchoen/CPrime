#ifndef _CPC_REGEX_NATIVE
#define _CPC_REGEX_NATIVE
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void* __cpc_regex_compile(const void* pattern, size_t length, int width,
                          int ignore_case, int multiline, char* error, int error_size);
void* __cpc_regex_clone(const void* program);
void __cpc_regex_free(void* program);
int __cpc_regex_capture_count(const void* program);
int __cpc_regex_search(const void* program, const void* text, size_t length,
                       int width, int continuous, int full, int* offsets);
#ifdef __cplusplus
}
#endif
#endif
