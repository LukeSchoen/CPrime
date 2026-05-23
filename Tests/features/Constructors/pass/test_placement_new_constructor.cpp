// EXPECT_EXIT: 0
struct Widget
{
  int value;

  Widget(int seed)
  {
    this->value = seed + 1;
  }
};

int main(void)
{
  char storage[sizeof(struct Widget)];
  struct Widget *w = new (storage) Widget(6);
  if (w != (struct Widget *)storage)
    return 1;
  return w->value == 7 ? 0 : 2;
}
