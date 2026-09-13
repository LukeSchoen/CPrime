// PERF_NAME: c.many.functions
/* Scale of many small function bodies: macro expansion, symbol registration,
   per-function code generation and a relocation-heavy call table. */

#define BODY(n, op)                                   \
  int fn_##n(int x, int y)                            \
  {                                                   \
    int a = x op y;                                   \
    int b = a op (x - y);                             \
    int c = (b * 3) op (a + (n));                     \
    return c + (a ^ b) - (n);                         \
  }

BODY(0, +) BODY(1, -) BODY(2, +) BODY(3, -)
BODY(4, +) BODY(5, -) BODY(6, +) BODY(7, -)
BODY(8, +) BODY(9, -) BODY(10, +) BODY(11, -)
BODY(12, +) BODY(13, -) BODY(14, +) BODY(15, -)
BODY(16, +) BODY(17, -) BODY(18, +) BODY(19, -)
BODY(20, +) BODY(21, -) BODY(22, +) BODY(23, -)
BODY(24, +) BODY(25, -) BODY(26, +) BODY(27, -)
BODY(28, +) BODY(29, -) BODY(30, +) BODY(31, -)
BODY(32, +) BODY(33, -) BODY(34, +) BODY(35, -)
BODY(36, +) BODY(37, -) BODY(38, +) BODY(39, -)
BODY(40, +) BODY(41, -) BODY(42, +) BODY(43, -)
BODY(44, +) BODY(45, -) BODY(46, +) BODY(47, -)

typedef int (*fn_ptr)(int, int);

static fn_ptr table[48] = {
  fn_0, fn_1, fn_2, fn_3, fn_4, fn_5, fn_6, fn_7,
  fn_8, fn_9, fn_10, fn_11, fn_12, fn_13, fn_14, fn_15,
  fn_16, fn_17, fn_18, fn_19, fn_20, fn_21, fn_22, fn_23,
  fn_24, fn_25, fn_26, fn_27, fn_28, fn_29, fn_30, fn_31,
  fn_32, fn_33, fn_34, fn_35, fn_36, fn_37, fn_38, fn_39,
  fn_40, fn_41, fn_42, fn_43, fn_44, fn_45, fn_46, fn_47
};

int main(void)
{
  int i;
  int total = 0;
  for (i = 0; i < 48; i++)
    total += table[i](i, i + 1);
  return total == 0 ? 1 : 0;
}
