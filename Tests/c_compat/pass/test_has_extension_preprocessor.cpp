#define FEAT(x) (__has_feature(x) && __has_extension(x))

#if __has_feature(cxx_constexpr_string_builtins)
#error unsupported feature reported
#endif

#if FEAT(cxx_constexpr_string_builtins)
#error unsupported extension reported
#endif

int main()
{
  return 0;
}
