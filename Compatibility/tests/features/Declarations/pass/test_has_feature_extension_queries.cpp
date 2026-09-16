// __has_feature()/__has_extension() answer for the C++ language features that
// the compiler accepts, while exceptions and RTTI stay unknown because the
// matching __cpp_* predefines are not emitted.
#if !defined(__has_feature) || !defined(__has_extension)
#error capability macros undefined
#endif
#if !__has_feature(cxx_lambdas) || !__has_extension(cxx_lambdas)
#error c++11 feature unavailable
#endif
#if !__has_feature(cxx_binary_literals) || !__has_extension(cxx_binary_literals)
#error c++14 feature unavailable
#endif
#if !__has_feature(cxx_variadic_templates) || !__has_extension(cxx_variadic_templates)
#error variadic templates feature unavailable
#endif
#if __has_feature(cxx_exceptions) || __has_extension(cxx_exceptions)
#error exceptions feature reported without __cpp_exceptions
#endif
#if !__has_extension(gnu_asm_goto_with_outputs) || __has_feature(gnu_asm_goto_with_outputs)
#error extension-only feature misreported
#endif
#if __has_feature(unknown_feature) || __has_extension(unknown_feature)
#error unknown feature reported
#endif

int main()
{
    return __has_feature(cxx_lambdas) ? 0 : 1;
}
