// EXPECT_EXIT: 0
typedef struct
{
  int i;
} Base;

struct A : public Base
{
  struct
  {
    int i;
  } x;
};

int main()
{
  A a;
  a.i = 5;
  a.x.i = 42;
  A b(a);
  a = b;
  return a.i == 5 && a.x.i == 42 ? 0 : 1;
}
