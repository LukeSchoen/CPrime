#include <cstdio>

using namespace std;

int main()
{
  FILE* stream = fopen(__FILE__, "rb");
  if (!stream)
    return 1;
  char buffer[4];
  size_t count = fread(buffer, 1, sizeof(buffer), stream);
  return fclose(stream) == 0 && count == sizeof(buffer) ? 0 : 2;
}
