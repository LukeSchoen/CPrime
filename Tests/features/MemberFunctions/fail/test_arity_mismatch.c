// EXPECT_COMPILE_FAIL: 1
typedef struct
{
  int value;
} Counter;

void add(Counter *c, int delta)
{
  c->value += delta;
}

int main(void)
{
  Counter c = {0};
  c.add();
  return 0;
}
