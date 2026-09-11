// EXPECT_EXIT: 0
/* GNU vector extensions: braced and compound-literal initialization,
   element subscript, element-wise arithmetic/logical operations and the
   convert/shuffle builtins. */
typedef int v4si __attribute__((vector_size(4 * sizeof(int))));
typedef float v4sf __attribute__((vector_size(4 * sizeof(float))));

int main() {
  v4si a = {8, 6, 4, 2};
  v4si b = {2, 3, 4, 5};
  v4si sum = a + b;
  if (sum[0] != 10 || sum[1] != 9 || sum[2] != 8 || sum[3] != 7)
    return 1;
  v4si quot = a / b;
  if (quot[0] != 4 || quot[1] != 2 || quot[2] != 1 || quot[3] != 0)
    return 2;
  v4si scaled = 3 * b;
  if (scaled[0] != 6 || scaled[3] != 15)
    return 3;
  v4si both = a && b;
  if (both[0] != -1 || both[1] != -1 || both[3] != -1)
    return 4;
  v4si none = (v4si){0, 0, 0, 0} || 0;
  if (none[0] != 0 || none[3] != 0)
    return 5;
  if (sizeof(a) != 16)
    return 6;
  v4si lit = (v4si){1, 2, 3, 4};
  if (lit[2] != 3)
    return 7;
  v4si rev = __builtin_shuffle(a, (v4si){3, 2, 1, 0});
  if (rev[0] != 2 || rev[3] != 8)
    return 8;
  v4si pick = __builtin_shufflevector(a, b, 0, 4, 1, 5);
  if (pick[0] != 8 || pick[1] != 2 || pick[2] != 6 || pick[3] != 3)
    return 9;
  v4sf fa = {12.0f, 8.0f, 4.0f, 2.0f};
  v4sf fb = {2.0f, 4.0f, 8.0f, 1.0f};
  v4sf fq = fa / fb;
  if (fq[0] != 6.0f || fq[2] != 0.5f || fq[3] != 2.0f)
    return 10;
  v4sf converted = __builtin_convertvector(a, v4sf);
  if (converted[0] != 8.0f || converted[3] != 2.0f)
    return 11;
  return 0;
}
