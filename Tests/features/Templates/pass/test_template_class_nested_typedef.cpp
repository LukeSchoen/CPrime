// EXPECT_EXIT: 0

template<typename T>
class Box
{
  typedef T Value;
  Value value;

  void set(Value next)
  {
    this->value = next;
  }

  Value get()
  {
    return this->value;
  }
};

int main(void)
{
  Box<int> box;
  Box<float> fbox;
  box.set(42);
  fbox.set(1.5f);
  if (box.get() != 42)
    return 1;
  if (fbox.get() != 1.5f)
    return 2;
  return 0;
}
