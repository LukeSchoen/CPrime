#define ATTRIBUTE unused
#define ALIAS ATTRIBUTE
#define SCOPED gnu::ALIAS
#if !defined(__has_builtin) || !__has_attribute(ALIAS) || !__has_attribute(SCOPED)
#error capability query macro expansion failed
#endif
#if __has_attribute(other::unused) || __has_attribute(gnu::unknown_attribute)
#error unknown attribute reported supported
#endif
#if !__has_builtin(__builtin_va_start) || !__has_builtin(__builtin_va_arg) || !__has_builtin(__builtin_va_end)
#error varargs builtins unavailable
#endif
#if !__has_builtin(__builtin_expect) || __has_builtin(__builtin_unknown_operation)
#error builtin query did not reflect support
#endif
int main() {
    return !__has_attribute(SCOPED) || !__has_attribute(__unused__)
        || __has_attribute(other::unused) || !__has_builtin(__builtin_expect)
        || __builtin_expect(42, 42) != 42;
}
