// The Itanium ABI header must provide the declaration set used here. The
// runtime keeps its own type_info representation.
#include <cxxabi.h>
#include <typeinfo>

static_assert(abi::__pbase_type_info::__const_mask == 0x1, "const mask");
static_assert(abi::__pbase_type_info::__volatile_mask == 0x2, "volatile mask");
static_assert(abi::__pbase_type_info::__incomplete_class_mask == 0x10,
              "incomplete class mask");

int main()
{
    const abi::__class_type_info *class_info = 0;
    const abi::__pointer_type_info *pointer_info = 0;
    const abi::__vmi_class_type_info *vmi_info = 0;
    return class_info != 0 || pointer_info != 0 || vmi_info != 0
        || sizeof(abi::__pointer_type_info) == 0;
}
