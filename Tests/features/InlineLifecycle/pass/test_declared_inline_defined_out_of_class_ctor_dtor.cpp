// EXPECT_EXIT: 0
int ctor_calls;
int dtor_sum;

struct Tracker
{
  int value;

  Tracker();
  ~Tracker();
};

Tracker::Tracker()
{
  this->value = 11;
  ctor_calls += 1;
}

Tracker::~Tracker()
{
  dtor_sum += this->value;
}

int run_scope(void)
{
  struct Tracker t;
  if (ctor_calls != 1)
    return 1;
  if (t.value != 11)
    return 2;
  return 9;
}

int main(void)
{
  int r = run_scope();
  if (r != 9)
    return 3;
  if (dtor_sum != 11)
    return 4;
  return 0;
}
