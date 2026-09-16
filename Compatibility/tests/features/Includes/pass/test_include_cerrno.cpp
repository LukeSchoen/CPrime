#include <cerrno>

int main()
{
  errno = 0;
  return errno;
}
