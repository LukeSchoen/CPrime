// EXPECT_EXIT: 0
struct Plain
{
  int v;
};

int main(void)
{
  struct Plain p = {11};
  return p.v == 11 ? 0 : 1;
}
