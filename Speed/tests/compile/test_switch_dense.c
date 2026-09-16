// PERF_NAME: c.switch.dense
/* Dense and sparse switch statements: jump tables, case ordering and the
   code generator's branch lowering. */

#define CASE(n) case n: return (n) * 3 - (n) / 2 + ((n) & 7);

static int classify(int value)
{
  switch (value) {
  CASE(0) CASE(1) CASE(2) CASE(3) CASE(4) CASE(5) CASE(6) CASE(7)
  CASE(8) CASE(9) CASE(10) CASE(11) CASE(12) CASE(13) CASE(14) CASE(15)
  CASE(16) CASE(17) CASE(18) CASE(19) CASE(20) CASE(21) CASE(22) CASE(23)
  CASE(24) CASE(25) CASE(26) CASE(27) CASE(28) CASE(29) CASE(30) CASE(31)
  default: return -1;
  }
}

static int sparse(int value)
{
  switch (value) {
  case 0: return 11;
  case 3: return 12;
  case 17: return 13;
  case 64: return 14;
  case 129: return 15;
  case 1000: return 16;
  case 4096: return 17;
  case 99991: return 18;
  default: return 0;
  }
}

int main(void)
{
  int i;
  int total = 0;
  for (i = 0; i < 2048; i++) {
    total += classify(i & 31);
    total += sparse(i & 4095);
    switch (i % 5) {
    case 0: total += 1; break;
    case 1: total += 2; break;
    case 2: total += 3; break;
    case 3: total += 4; break;
    default: total -= 5; break;
    }
  }
  return total == 0 ? 1 : 0;
}
