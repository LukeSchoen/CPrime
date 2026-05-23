typedef enum ModeTag { ModeA = 1, ModeB = 2 } Mode;

struct Device
{
  Mode mode;

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