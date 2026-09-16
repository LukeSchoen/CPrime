/* __has_builtin answers for C translation units: the C-only type-compatible
   spelling is available, the C++ traits and cast helpers are not. */
#if !__has_builtin(__builtin_types_compatible_p)
#error missing c-only builtin
#endif
#if __has_builtin(__is_pod) || __has_builtin(__has_trivial_assign)
#error c++ trait reported for c
#endif
#if __has_builtin(__builtin_addressof) || __has_builtin(__builtin_launder)
#error c++ cast builtin reported for c
#endif
#if __has_builtin(__builtin_is_constant_evaluated)
#error c++ constant-evaluated builtin reported for c
#endif
#if !__has_builtin(__builtin_abs) || !__has_builtin(abs)
#error library builtin unavailable
#endif
#if !__has_builtin(__builtin_object_size) || !__has_builtin(__sync_synchronize)
#error common builtin unavailable
#endif

int main(void)
{
    return 0;
}
