/* The flag 3 linemarker marks this text as a system header.  GCC tolerates
   bool and wchar_t typedefs there without letting them replace the builtins. */
# 1 "test_system_header_builtin_typedefs.h" 1 3
typedef int bool;
typedef int wchar_t;
# 2 "test_system_header_builtin_typedefs.cpp" 2
# 1 "test_system_header_long_wchar_typedef.h" 1 3
typedef long wchar_t;
# 2 "test_system_header_builtin_typedefs.cpp" 2

int main()
{
  if (sizeof(bool) != 1)
    return 1;
  if (sizeof(wchar_t) != 2)
    return 2;

  bool value = true;
  wchar_t wide = L'x';
  return value && wide == L'x' ? 0 : 3;
}
