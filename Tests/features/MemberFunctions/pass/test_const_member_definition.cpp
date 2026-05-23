// EXPECT_EXIT: 0
struct Counter
{
  int value;

  int get() const;
};

int Counter::get() const
{
  return this->value;
}

int main(void)
{
  struct Counter c;
  c.value = 42;
  return c.get() == 42 ? 0 : 1;
}
