// EXPECT_STDOUT: 20
// EXPECT_MANIFEST_SOURCE: CommonLib/CommonLib/src/Strings/clString.cpp
#include "clScan.h"
#include "clSort.h"
#include "clStream.h"

#include <stdlib.h>
#include <string.h>

// Minimal runtime allocator stubs so the focused test links standalone.
void *_clAlloc(int lineNumber, const char *fileName, i64 size, bool zeroMemory)
{
  (void)lineNumber;
  (void)fileName;
  void *p = malloc((size_t)size);
  if (p && zeroMemory)
    memset(p, 0, (size_t)size);
  return p;
}

void _clFree(const void *pData)
{
  free((void *)pData);
}

int main()
{
  clList<char> buf;
  buf.Resize(10);
  clList<char> buf2;
  buf2.Resize(10, 'x');
  printf("%d", (int)(buf.Size() + buf2.Size()));
  return 0;
}
