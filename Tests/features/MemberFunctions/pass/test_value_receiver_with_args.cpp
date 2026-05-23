// EXPECT_EXIT: 0
// EXPECT_STDOUT:
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
  Counter c = {10};
  c.add(5);
  return c.value == 15 ? 0 : 1;
}
