// The C++ trait and cast builtins are reported to the preprocessor only for
// C++ translation units; C-only spellings stay hidden.
#if !__has_builtin(__is_pod) || !__has_builtin(__is_same)
#error missing c++ classification builtin
#endif
#if !__has_builtin(__has_trivial_destructor) || !__has_builtin(__has_nothrow_assign)
#error missing c++ property builtin
#endif
#if !__has_builtin(__remove_cvref) || !__has_builtin(__underlying_type)
#error missing c++ transformation builtin
#endif
#if !__has_builtin(__builtin_addressof) || !__has_builtin(__builtin_launder)
#error missing c++ cast builtin
#endif
#if !__has_builtin(__builtin_is_constant_evaluated)
#error missing c++ constant-evaluated builtin
#endif
#if __has_builtin(__builtin_types_compatible_p)
#error c-only builtin reported for c++
#endif
#if __has_builtin(__alignof__) || __has_builtin(asm) || __has_builtin(__typeof__)
#error keyword reported as builtin
#endif

int main()
{
    return 0;
}
