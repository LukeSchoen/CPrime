// EXPECT_EXIT: 0

typedef unsigned int Uint32;
typedef int Sint32;

struct Foo
{
  Uint32 first;
  Sint32 second;
};

static int check(struct Foo f)
{
  return f.first == 1 && f.second == -1;
}

int main(void)
{
  struct Foo f;
  f.first = 1;
  f.second = -1;
  return check(f) ? 0 : 1;
}
