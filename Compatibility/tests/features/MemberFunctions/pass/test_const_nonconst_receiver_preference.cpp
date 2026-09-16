// EXPECT_EXIT: 0
struct Box
{
  int value;
  int get();
  int get() const;
};

int Box::get()
{
  return this->value + 1;
}

int Box::get() const
{
  return this->value + 2;
}

int main(void)
{
  Box mutable_box = {10};
  const Box const_box = {10};
  return mutable_box.get() == 11 && const_box.get() == 12 ? 0 : 1;
}

