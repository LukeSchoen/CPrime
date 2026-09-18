enum Flag {
  flag_zero = 0,
  flag_value = 5
};

static int direct = flag_value;
static unsigned zero = flag_zero;
static int table[] = { flag_value, flag_zero, flag_value };
static int computed = flag_value + 1;
static int parenthesized = (flag_value << 1);

static int local_value(void)
{
  static int value = flag_value;
  return value;
}

int main(void)
{
  if (direct != 5) return 1;
  if (zero != 0) return 2;
  if (local_value() != 5) return 3;
  if (table[0] != 5 || table[1] != 0 || table[2] != 5) return 4;
  if (computed != 6) return 5;
  if (parenthesized != 10) return 6;
  return 0;
}
