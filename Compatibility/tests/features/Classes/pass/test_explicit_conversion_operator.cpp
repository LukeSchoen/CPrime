struct Number
{
  int value;

  explicit operator int() const
  {
    return value;
  }
};

int main()
{
  Number number;
  number.value = 42;
  return number.value == 42 ? 0 : 1;
}
