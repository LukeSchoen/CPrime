// EXPECT_EXIT: 0
struct A
{
  int i[0];
  int j;
};

static A a = { 1 };

int main()
{
  return a.j == 1 ? 0 : 1;
}
