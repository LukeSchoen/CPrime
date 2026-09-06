#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "cprime_regex.h"
#include "libregexp.h"

typedef struct RegexContext { uintptr_t stack_low; } RegexContext;
typedef struct RegexProgram {
    uint8_t *search, *full;
    int search_length, full_length, captures;
} RegexProgram;

static RegexContext regex_context(void)
{
    RegexContext context;
    MEMORY_BASIC_INFORMATION info;
    VirtualQuery(&context, &info, sizeof(info));
    context.stack_low = (uintptr_t)info.AllocationBase;
    return context;
}

int lre_check_stack_overflow(void* opaque, size_t size)
{
    RegexContext* context = opaque;
    uintptr_t current = (uintptr_t)&context;
    return current < context->stack_low || size > current - context->stack_low
        || current - context->stack_low - size < 32768;
}

int lre_check_timeout(void* opaque) { (void)opaque; return 0; }
void* lre_realloc(void* opaque, void* pointer, size_t size)
{
    (void)opaque;
    if (!size) { free(pointer); return NULL; }
    return realloc(pointer, size);
}

void __cpc_regex_free(void* opaque)
{
    RegexProgram* program = opaque;
    if (!program) return;
    free(program->search);
    free(program->full);
    free(program);
}

void* __cpc_regex_compile(const void* pattern, size_t length, int width,
                          int ignore_case, int multiline, char* error, int error_size)
{
    RegexProgram* program;
    RegexContext context = regex_context();
    char* encoded;
    size_t i, at = 3;
    int flags = (ignore_case ? LRE_FLAG_IGNORECASE : 0)
              | (multiline ? LRE_FLAG_MULTILINE : 0);
    if ((width != 1 && width != 2) || length > (INT_MAX - 16) / 3) return NULL;
    encoded = malloc(length * 3 + 16);
    if (!encoded) return NULL;
    memcpy(encoded, "(?:", 3);
    for (i = 0; i < length; ++i) {
        unsigned value = width == 1 ? ((const unsigned char*)pattern)[i]
                                   : ((const unsigned short*)pattern)[i];
        if (value < 0x80) encoded[at++] = value;
        else if (value < 0x800) {
            encoded[at++] = 0xc0 | (value >> 6);
            encoded[at++] = 0x80 | (value & 63);
        } else {
            encoded[at++] = 0xe0 | (value >> 12);
            encoded[at++] = 0x80 | ((value >> 6) & 63);
            encoded[at++] = 0x80 | (value & 63);
        }
    }
    program = calloc(1, sizeof(*program));
    if (!program) { free(encoded); return NULL; }
    /* The engine accepts an explicit length but also requires a sentinel. */
    encoded[at] = '\0';
    program->search = lre_compile(&program->search_length, error, error_size,
                                   encoded + 3, at - 3, flags, &context);
    if (program->search) {
        /* A full match must allow alternatives to backtrack through the end
           assertion, rather than rejecting the first shorter search result. */
        memcpy(encoded + at, ")(?![\\s\\S])", 12);
        at += 11;
        program->full = lre_compile(&program->full_length, error, error_size,
                                     encoded, at, flags | LRE_FLAG_STICKY, &context);
    }
    free(encoded);
    if (!program->search || !program->full) { __cpc_regex_free(program); return NULL; }
    program->captures = lre_get_capture_count(program->search);
    return program;
}

void* __cpc_regex_clone(const void* opaque)
{
    const RegexProgram* source = opaque;
    RegexProgram* result;
    if (!source) return NULL;
    result = calloc(1, sizeof(*result));
    if (!result) return NULL;
    *result = *source;
    result->search = malloc(source->search_length);
    result->full = malloc(source->full_length);
    if (!result->search || !result->full) { __cpc_regex_free(result); return NULL; }
    memcpy(result->search, source->search, source->search_length);
    memcpy(result->full, source->full, source->full_length);
    return result;
}

int __cpc_regex_capture_count(const void* opaque)
{
    const RegexProgram* program = opaque;
    return program ? program->captures : 0;
}

int __cpc_regex_search(const void* opaque, const void* text, size_t length,
                       int width, int continuous, int full, int* offsets)
{
    const RegexProgram* program = opaque;
    RegexContext context = regex_context();
    uint8_t* captures[510];
    int result, i;
    if (!program) return 0;
    if (length > INT_MAX || (width != 1 && width != 2)) return -1;
    result = lre_exec(captures, full ? program->full : program->search,
                       text, 0, (int)length, width == 2, &context);
    if (result <= 0) return result;
    if (continuous && captures[0] != (const uint8_t*)text) return 0;
    for (i = 0; i < program->captures * 2; ++i)
        offsets[i] = captures[i] ? (int)((captures[i] - (const uint8_t*)text) / width) : -1;
    return 1;
}
