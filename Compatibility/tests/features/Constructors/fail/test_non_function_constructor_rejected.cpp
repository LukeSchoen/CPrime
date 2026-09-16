// EXPECT_COMPILE_FAIL: 1

struct Bad
{
  int v;
};

Bad::Bad()
{
  this->v = 1;
}

int main(void)
{
  struct Bad b = {0};
  return b.v;
}
