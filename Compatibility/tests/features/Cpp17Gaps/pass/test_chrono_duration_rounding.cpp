// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: chrono_rounding. <chrono> lacks floor, ceil and round for
// durations, and a duration does not convert to a floating-point duration.

#include <chrono>

int main()
{
  using namespace std::chrono;
  if (floor<seconds>(milliseconds(1500)) != seconds(1)) return 1;
  if (ceil<seconds>(milliseconds(1500)) != seconds(2)) return 2;
  if (round<seconds>(milliseconds(1500)) != seconds(2)) return 3;
  return duration<double>(milliseconds(1500)).count() > 1.4 ? 0 : 4;
}
