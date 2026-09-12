// EXPECT_EXIT: 0
// The Itanium ABI describes a pointer typeid through __pointer_type_info: the
// pointee's top-level qualifiers live in __flags and __pointee names the
// pointee's own descriptor.
#include <cxxabi.h>
#include <typeinfo>

int main()
{
  const abi::__pointer_type_info &array_pointer =
    static_cast<const abi::__pointer_type_info &>(typeid(const int (*)[3]));
  if ((array_pointer.__flags & abi::__pbase_type_info::__const_mask) == 0) return 1;
  if (array_pointer.__pointee != &typeid(const int [3])) return 2;

  const abi::__pointer_type_info &plain =
    static_cast<const abi::__pointer_type_info &>(typeid(int *));
  if (plain.__flags != 0 || plain.__pointee != &typeid(int)) return 3;

  const abi::__pointer_type_info &constant =
    static_cast<const abi::__pointer_type_info &>(typeid(int const *));
  if ((constant.__flags & abi::__pbase_type_info::__const_mask) == 0) return 4;
  if (constant.__pointee != &typeid(int)) return 5;

  const abi::__pointer_type_info &observable =
    static_cast<const abi::__pointer_type_info &>(typeid(long volatile *));
  if ((observable.__flags & abi::__pbase_type_info::__volatile_mask) == 0) return 6;

  const abi::__pointer_type_info &nested =
    static_cast<const abi::__pointer_type_info &>(typeid(int **));
  if (nested.__flags != 0 || nested.__pointee != &typeid(int *)) return 7;

  const abi::__pointer_type_info &empty =
    static_cast<const abi::__pointer_type_info &>(typeid(void *));
  if (empty.__flags != 0 || empty.__pointee != &typeid(void)) return 8;
  return 0;
}
