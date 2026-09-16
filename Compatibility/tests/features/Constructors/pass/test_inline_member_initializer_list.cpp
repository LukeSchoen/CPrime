// EXPECT_EXIT: 0

class Pair
{
public:
  int a;
  int b;

  Pair(int x, int y) : a(x), b(y) {}
};

int main(void)
{
  Pair p(2, 5);
  return p.a == 2 && p.b == 5 ? 0 : 1;
}
