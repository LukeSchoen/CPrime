/* CPC packaging wrapper for the Microsoft GDI+ headers.

   The Windows headers under gdiplus/ are retained verbatim.  GDI+ is not
   standalone in the Windows SDK: it expects the SAL annotations, COM base
   types and the usual min/max helpers to be visible already.  This wrapper
   supplies those prerequisites and keeps the definitions local to the GDI+
   include so ordinary code after <gdiplus.h> does not inherit min/max macros
   or a global byte typedef. */
#ifndef _CPRIME_GDIPLUS_H
#define _CPRIME_GDIPLUS_H

#include <sal.h>
#include <objidl.h>
#include <algorithm>

#ifndef MIDL_INTERFACE
#define MIDL_INTERFACE(x) struct
#endif

#ifndef _CPRIME_PROPID_DEFINED
#define _CPRIME_PROPID_DEFINED
typedef unsigned long PROPID;
#endif

struct IStream;

namespace Gdiplus
{
using std::min;
using std::max;
typedef unsigned char byte;
}

#include "gdiplus/gdiplus.h"

#endif
