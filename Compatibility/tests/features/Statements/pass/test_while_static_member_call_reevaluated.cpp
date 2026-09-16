static int UpdateCalls;

struct LoopControls
{
  static bool Update(const bool &keepRunning = true)
  {
    ++UpdateCalls;
    return keepRunning && UpdateCalls < 4;
  }
};

int main()
{
  int bodyCalls = 0;
  while (LoopControls::Update())
  {
    ++bodyCalls;
    if (bodyCalls > 8)
      return 1;
  }
  return UpdateCalls == 4 && bodyCalls == 3 ? 0 : 2;
}
