// EXPECT_EXIT: 0
struct Gadget
{
  int value;

  Gadget(int seed);
};

int main(void)
{
  struct Gadget gadget(9);
  return gadget.value == 9 ? 0 : 1;
}

Gadget::Gadget(int seed)
{
  this->value = seed;
}
