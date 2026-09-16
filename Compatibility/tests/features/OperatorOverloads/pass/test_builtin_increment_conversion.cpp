// EXPECT_EXIT: 0
long value = 7;
int conversions;
struct Scalar {
  operator long &() { ++conversions; return value; }
};
struct Inherited : Scalar {};
int main() {
  Inherited x;
  if (x++ != 7 || value != 8) return 1;
  if (++x != 9 || value != 9) return 2;
  if (x-- != 9 || value != 8) return 3;
  if (--x != 7 || value != 7 || conversions != 4) return 4;
  return 0;
}
