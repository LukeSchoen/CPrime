#ifndef _CPC_NTVERP
#define _CPC_NTVERP

/* The vendored Windows SDK headers are the mingw-w64 Win32 surface, which
   predates the Windows Runtime.  ntverp.h reports the SDK build that produced
   those headers, so boost/predef/platform/windows_uwp.h keeps UWP detection
   off instead of reaching for the WinRT-only headers this SDK does not carry.
   Windows 7's SDK is the newest one whose API set matches the vendored tree. */
#define VER_PRODUCTBUILD         7600
#define VER_PRODUCTBUILD_QFE     16385
#define VER_PRODUCTMAJORVERSION  6
#define VER_PRODUCTMINORVERSION  1
#define VER_PRODUCTMINORVERSION_STR "1"

#define VER_PRODUCTVERSION_MAJOR VER_PRODUCTMAJORVERSION
#define VER_PRODUCTVERSION_MINOR VER_PRODUCTMINORVERSION
#define VER_PRODUCTVERSION_BUILD VER_PRODUCTBUILD
#define VER_PRODUCTVERSION_QFE   VER_PRODUCTBUILD_QFE
#define VER_PRODUCTVERSION \
  VER_PRODUCTVERSION_MAJOR,VER_PRODUCTVERSION_MINOR,VER_PRODUCTVERSION_BUILD,VER_PRODUCTVERSION_QFE
#define VER_PRODUCTVERSION_STR   "6.1.7600.16385"

#endif
