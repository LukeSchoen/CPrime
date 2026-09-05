// EXPECT_COMPILE_ONLY: 1
// EXPECT_MANIFEST_SOURCE: CommonLib/CommonLib/src/Strings/clString.cpp
#include "cpcList.h"
#include "clList.h"

extern void accepts_size_data(long long, const void *);

void call_size_data(cpcList<ui8> &buffer)
{
  accepts_size_data(buffer.Size(), buffer.Data());
}

void call_cl_size_data(clList<ui8> &buffer)
{
  accepts_size_data(buffer.Size(), buffer.Data());
}

int main()
{
  return 0;
}
