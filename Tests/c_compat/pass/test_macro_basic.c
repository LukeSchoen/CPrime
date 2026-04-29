// EXPECT_EXIT: 0
// EXPECT_STDOUT:
#define MUL2(x) ((x) * 2)

int main(void)
{
  int a = 5;
  return MUL2(a) == 10 ? 0 : 1;
}
