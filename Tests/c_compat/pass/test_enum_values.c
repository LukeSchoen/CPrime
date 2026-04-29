// EXPECT_EXIT: 0
// EXPECT_STDOUT:
enum Color
{
  RED = 1,
  GREEN = 2,
  BLUE = 4
};

int main(void)
{
  enum Color c = GREEN;
  return c == 2 ? 0 : 1;
}
