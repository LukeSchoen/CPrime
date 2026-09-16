// EXPECT_EXIT: 0
int g;

struct Box
{
  int v;

  ~Box();
};

Box::~Box()
{
  g += this->v;
}

int main(void)
{
  {
    struct Box b = {3};
  }
  return g == 3 ? 0 : 1;
}
