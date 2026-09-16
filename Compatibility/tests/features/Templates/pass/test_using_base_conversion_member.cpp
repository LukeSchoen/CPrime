// EXPECT_EXIT: 0
struct Target
{
  int value;
};

static Target base_target;
static Target derived_target;

struct Base
{
  operator Target *() const { return &base_target; }
};

struct Wrapper : Base
{
  using Base::operator Target*;
  operator Target *() const { return &derived_target; }

  int check() const { return operator Target*() != &derived_target; }
};

int main()
{
  Wrapper wrapper;
  return wrapper.check();
}
