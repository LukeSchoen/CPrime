#include "core/Containers/cpcList.h"

extern void accepts_void(const void *);

void call_data(cpcList<ui8> &buffer)
{
  accepts_void(buffer.Data());
}

int main()
{
  return 0;
}
