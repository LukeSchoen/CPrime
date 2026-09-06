// EXPECT_COMPILE_ONLY: 1
// EXPECT_MANIFEST_SOURCE: CommonLib/CommonLib/src/Strings/clString.cpp
#include "clList.h"

extern void accepts_void(const void *);

void call_data(clList<ui8> &buffer)
{
  accepts_void(buffer.Data());
}

void call_const_data(const clList<ui8> &buffer)
{
  accepts_void(buffer.Data());
}

int main()
{
  return 0;
}
