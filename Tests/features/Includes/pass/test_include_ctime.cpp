#include <ctime>

int main()
{
  return time(0) == (time_t)-1 ? 1 : 0;
}
