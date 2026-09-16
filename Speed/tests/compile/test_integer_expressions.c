// PERF_NAME: c.integer.expressions
/* Dense integer expression trees and inner loops: parser precedence handling,
   register allocation and the peephole passes. */

static unsigned mix(unsigned a, unsigned b)
{
  unsigned acc = a ^ b;
  acc = acc + (a << 3) - (b >> 2);
  acc = (acc * 2654435761u) ^ (acc >> 13);
  acc = acc - (-a) + (~b);
  acc = (acc | (b & 0xff00ff00u)) ^ (a + b);
  acc = acc ? (acc + 1) : (acc - 1);
  acc = (acc < a) ? (acc * 3) : (acc / 5);
  acc = ((acc % 7) + (acc % 11)) * ((acc % 13) + 1);
  return acc;
}

static int nested(int x, int y, int z)
{
  int a = (x + y) * (z - x) + (x * y - z);
  int b = ((a << 2) | (a >> 3)) ^ (y * z + x);
  int c = (a > b) ? (a - b) : (b - a);
  int d = (c & 0x5555) + (c | 0x0f0f) + (c ^ 0x3333);
  int e = (d % 97) * (d % 89) - (d % 83);
  return a + b + c + d + e;
}

int main(void)
{
  unsigned acc = 1;
  int i;
  int j;
  for (i = 0; i < 4096; i++) {
    acc = mix(acc + (unsigned)i, acc ^ (unsigned)i);
    for (j = 0; j < 4; j++)
      acc += (unsigned)nested(i, j, (int)(acc & 31));
  }
  return acc == 0 ? 1 : 0;
}
