// EXPECT_EXIT: 0

class A
{
public:
  const char *f()
  {
    return "x";
  }
};

int main(void)
{
  A a;
  return a.f()[0] == 'x' ? 0 : 1;
}
