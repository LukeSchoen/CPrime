// EXPECT_EXIT: 0
int dtor_count;

struct Inner
{
  ~Inner()
  {
    dtor_count += 1;
  }
};

struct Outer
{
  Inner inner;
};

int main(void)
{
  {
    struct Outer outer;
  }
  return dtor_count == 1 ? 0 : 1;
}
