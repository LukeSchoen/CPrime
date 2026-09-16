// boost/predef/platform/windows_uwp.h includes <ntverp.h> unconditionally on
// a Windows target.  The vendored Windows SDK predates the Windows Runtime,
// so the header reports a pre-UWP build and leaves UWP detection off.
#include <ntverp.h>

int main()
{
  if (VER_PRODUCTBUILD <= 0) return 1;
  if (VER_PRODUCTBUILD >= 9200) return 2;
  if (VER_PRODUCTMAJORVERSION <= 0) return 3;
  return 0;
}
