#include "core/Containers/cpcList.h"

extern void accepts_size_data(long long, const void *);

void call_size_data(cpcList<ui8> &buffer)
{
  accepts_size_data(buffer.Size(), buffer.Data());
}

int main()
{
  return 0;
}
