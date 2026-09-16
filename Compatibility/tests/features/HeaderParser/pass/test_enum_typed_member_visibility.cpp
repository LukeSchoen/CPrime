#include <stdio.h>

enum Mode { ModeA = 1, ModeB = 2 };

struct Device
{
  enum Mode mode;

  int readMode(void)
  {
    return mode == ModeA ? 1 : 0;
  }
};

int main(void)
{
  struct Device d;
  d.mode = ModeB;
  return d.readMode();
}