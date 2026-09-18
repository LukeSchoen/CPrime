// <cxxabi.h> publishes the Itanium demangler, and code that names it --
// boost::core::demangle is the common one -- has to compile and link, so the
// entry point exists in the runtime.  This target mangles with the Microsoft
// x64 ABI, so a name handed to it never demangles and the caller keeps the
// spelling it passed.
#include <cxxabi.h>
#include <cstddef>
#include <cstdlib>

int main()
{
  int status = 0;
  std::size_t length = 0;
  char *demangled = abi::__cxa_demangle("_Z3fooi", 0, &length, &status);
  /* The ABI pairs a returned buffer with status 0.  A null result reports a
     name that does not demangle, which is what an Itanium entry point has to
     say about every name this ABI mangles. */
  if (demangled != 0 && status != 0)
    return 1;
  if (demangled == 0 && status == 0)
    return 2;
  if (demangled != 0)
    std::free(demangled);
  return 0;
}
