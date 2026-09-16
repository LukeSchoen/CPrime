// EXPECT_EXIT: 0
int calls;
int source() { ++calls; return 7; }
int main() {
  int value = -3;
  const unsigned &converted = static_cast<const unsigned &>(value);
  if (converted != static_cast<unsigned>(-3)) return 1;
  value = 4;
  if (converted != static_cast<unsigned>(-3)) return 2;
  const int &alias = static_cast<const int &>(value);
  value = 9;
  if (alias != 9) return 3;
  if (static_cast<const double &>(source()) != 7.0 || calls != 1) return 4;
  if (static_cast<const int &>(5) != 5) return 5;
  return 0;
}
