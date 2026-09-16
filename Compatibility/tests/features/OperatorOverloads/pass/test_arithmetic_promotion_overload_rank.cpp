int select(int) { return 1; }
int select(long) { return 2; }
int select(float) { return 3; }
int floating(double) { return 1; }
int floating(long double) { return 2; }
int reference(const int&) { return 1; }
int reference(const long&) { return 2; }

int main()
{
  signed char byte = -7;
  unsigned short small = 65535;
  if (select(byte) != 1 || select(small) != 1 || select(true) != 1) return 1;
  if (select('x') != 1 || select(L'x') != 1) return 2;
  if (select(4L) != 2 || select(0.5f) != 3) return 3;
  if (floating(0.5f) != 1) return 4;
  if (reference(byte) != 1 || reference(small) != 1) return 5;
  return 0;
}
