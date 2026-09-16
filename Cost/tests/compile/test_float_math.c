// PERF_NAME: c.float.math
/* Floating point conversion and code generation: int/double conversions,
   mixed arithmetic, comparisons and compound assignment. */

static double poly(double x, double a, double b, double c)
{
  return ((a * x + b) * x + c) * x;
}

static double accumulate(double seed)
{
  double acc = seed;
  int i;
  for (i = 0; i < 512; i++) {
    double t = (double)i;
    acc += poly(t, 1.0001, 2.0002, 3.0003);
    acc *= 1.0000001;
    acc -= t / 3.0;
    if (acc > 1000.0)
      acc = acc / 2.0;
    else if (acc < -1000.0)
      acc = acc * 2.0;
  }
  return acc;
}

int main(void)
{
  double total = 0.0;
  int i;
  for (i = 0; i < 8; i++)
    total += accumulate((double)i + 0.5);
  return total == 0.0 ? 1 : 0;
}
