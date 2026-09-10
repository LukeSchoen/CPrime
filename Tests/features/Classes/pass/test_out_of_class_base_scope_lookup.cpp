// EXPECT_EXIT: 0
namespace N1
{
  namespace N2
  {
    class A {};
    class B;
  }
}

class N1::N2::B : public A
{
};

class C1
{
  class A {};
  class B;
};

class C1::B : A
{
};

int main()
{
  N1::N2::B first;
  C1::B second;
  (void)first;
  (void)second;
  return 0;
}
