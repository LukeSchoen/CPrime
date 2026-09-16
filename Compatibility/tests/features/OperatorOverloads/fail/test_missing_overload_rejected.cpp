// EXPECT_COMPILE_FAIL: 1
struct Num
{
  int v;
};

int main(void)
{
  struct Num a = {1};
  struct Num b = {2};
  return a + b;
}

