// __has_feature / __has_extension / __has_builtin query semantics.

#define FEAT(x) (__has_feature(x) && __has_extension(x))

#if FEAT(cxx_exceptions) != !!__cpp_exceptions
#error cxx_exceptions does not match __cpp_exceptions
#endif

#if FEAT(cxx_rtti) != !!__cpp_rtti
#error cxx_rtti does not match __cpp_rtti
#endif

#if !__has_extension(cxx_alias_templates)
#error alias templates should be available
#endif

#if __has_feature(cxx_constexpr_string_builtins)
#error constexpr string builtins are not implemented
#endif

#if __has_extension(cxx_constexpr_string_builtins)
#error constexpr string builtins are not implemented
#endif

template <typename T> struct AliasProbe
{
  typedef T type;
};

template <typename T> using alias_of = AliasProbe<T>;

static int check_has_builtin()
{
  if (!__has_builtin(__builtin_trap)) { return 1; }
  if (!__has_builtin(__builtin_expect)) { return 1; }
  if (!__has_builtin(__builtin_alloca)) { return 1; }
  if (!__has_builtin(__builtin_unreachable)) { return 1; }
  if (__has_builtin(__builtin_cpc_missing_builtin)) { return 1; }
  /* The overflow family is not implemented yet, so it must not be
     advertised: callers feature-test it and then compile their own
     fallback, which would not build if the query answered true. */
  if (__has_builtin(__builtin_add_overflow)) { return 1; }
  if (__has_builtin(__builtin_mul_overflow)) { return 1; }
  return 0;
}

int main()
{
  alias_of<int>::type value = 0;
  return value == 0 ? check_has_builtin() : 1;
}
