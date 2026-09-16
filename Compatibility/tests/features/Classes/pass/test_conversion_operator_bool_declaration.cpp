struct Flag
{
  int value;
  operator bool() const;
};

Flag::operator bool() const
{
  return value != 0;
}

int main()
{
  Flag flag;
  flag.value = 1;
  return flag ? 0 : 1;
}
