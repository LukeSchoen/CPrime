struct Arrow
{
  int value;
  Arrow *operator->() { return this; }
  int read() const { return value; }
};

int main()
{
  Arrow arrow;
  arrow.value = 7;
  return arrow.operator->()->read() != 7;
}
