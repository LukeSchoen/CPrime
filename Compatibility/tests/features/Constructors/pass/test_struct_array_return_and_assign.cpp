// EXPECT_EXIT: 0

class Mat4
{
public:
  double v[16];
};

static Mat4 identity(void)
{
  Mat4 m;
  int i;
  for (i = 0; i < 16; i = i + 1)
    m.v[i] = 0.0;
  m.v[0] = 1.0;
  m.v[5] = 1.0;
  m.v[10] = 1.0;
  m.v[15] = 1.0;
  return m;
}

static Mat4 scale(double x, double y, double z)
{
  Mat4 m = identity();
  m.v[0] = x;
  m.v[5] = y;
  m.v[10] = z;
  return m;
}

static Mat4 mul(Mat4 a, Mat4 b)
{
  Mat4 out;
  int c;
  int r;
  int k;
  for (c = 0; c < 4; c = c + 1)
    for (r = 0; r < 4; r = r + 1)
    {
      double sum = 0.0;
      for (k = 0; k < 4; k = k + 1)
        sum = sum + a.v[k * 4 + r] * b.v[c * 4 + k];
      out.v[c * 4 + r] = sum;
    }
  return out;
}

int main(void)
{
  Mat4 a = identity();
  Mat4 b = scale(2.0, 3.0, 4.0);
  Mat4 c = mul(a, b);
  if (c.v[0] != 2.0)
    return 1;
  if (c.v[5] != 3.0)
    return 2;
  if (c.v[10] != 4.0)
    return 3;
  if (c.v[15] != 1.0)
    return 4;
  return 0;
}
