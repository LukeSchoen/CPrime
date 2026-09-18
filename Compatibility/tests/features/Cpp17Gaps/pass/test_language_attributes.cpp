// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: language_attributes. [[nodiscard]], [[maybe_unused]] and
// [[fallthrough]] parse, apply and keep the fallthrough case reachable; no
// retained case covered any of the three.

[[nodiscard]] int produced() { return 7; }

[[maybe_unused]] static int retained = 3;

int main()
{
  [[maybe_unused]] int local = produced();
  int result = 0;
  switch (produced()) {
  case 7:
    result = 1;
    [[fallthrough]];
  case 8:
    ++result;
    break;
  default:
    break;
  }
  return result == 2 && retained == 3 && local == 7 ? 0 : 1;
}
