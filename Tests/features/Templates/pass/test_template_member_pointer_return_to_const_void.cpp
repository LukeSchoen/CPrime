// EXPECT_COMPILE_ONLY: 1
// EXPECT_MANIFEST_SOURCE: CommonLib/CommonLib/src/Strings/clString.cpp
#include "cpcList.h"
#include "clList.h"

extern void accepts_void(const void *);

void call_data(cpcList<ui8> &buffer)
{
  accepts_void(buffer.Data());
}

void call_cl_data(clList<ui8> &buffer)
{
  accepts_void(buffer.Data());
}

int main()
{
  return 0;
}
