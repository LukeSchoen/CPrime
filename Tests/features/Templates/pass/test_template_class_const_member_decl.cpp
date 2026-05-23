// EXPECT_EXIT: 0

template<typename T>
class X
{
public:
  int empty() const;
  int a;
};

int main(void)
{
  X<int> x;
  x.a = 7;
  return x.a == 7 ? 0 : 1;
}
