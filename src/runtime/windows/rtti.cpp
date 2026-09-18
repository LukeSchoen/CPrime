#include <typeinfo>
#include <stddef.h>

extern "C" void *__cpc_dynamic_cast_reference(void *result)
{
    if (!result) throw std::bad_cast();
    return result;
}

/* <cxxabi.h> publishes the Itanium demangler, and code that names it through
   that header -- boost::core::demangle is the common one -- has to link.  This
   target mangles with the Microsoft x64 ABI, so an Itanium name never applies:
   report the documented invalid-name status and leave the caller with the
   spelling it was handed. */
extern "C" char *__cxa_demangle(const char *mangled_name, char *output_buffer,
                                size_t *length, int *status)
{
    (void)mangled_name;
    (void)output_buffer;
    (void)length;
    if (status) *status = -2;
    return 0;
}
