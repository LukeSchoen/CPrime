// EXPECT_EXIT: 0
int seq[4];
int n;

struct Tracked
{
  int id;

  ~Tracked();
};

Tracked::~Tracked()
{
  seq[n++] = this->id;
}

int main(void)
{
  {
    struct Tracked outer = {1};
    {
      struct Tracked inner = {2};
    }
    seq[n++] = 9;
  }

  return (n == 3 && seq[0] == 2 && seq[1] == 9 && seq[2] == 1) ? 0 : 1;
}
