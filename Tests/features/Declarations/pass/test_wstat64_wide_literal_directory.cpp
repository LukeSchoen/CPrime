#include <sys/stat.h>

int main()
{
  struct _stat64 status;
  return _wstat64(L".", &status) == 0 ? 0 : 1;
}
