// EXPECT_EXIT: 0
int seq[4];
int n;

struct Guard
{
  int id;

  ~Guard();
};

Guard::~Guard()
{
  seq[n++] = this->id;
}

int work(void)
{
  struct Guard outer = {1};
  {
    struct Guard inner = {2};
    return 7;
  }
}

int main(void)
{
  int r = work();
  return (r == 7 && n == 2 && seq[0] == 2 && seq[1] == 1) ? 0 : 1;
}
