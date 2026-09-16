[[deprecated("use current_value instead")]] int legacy_value();

int current_value()
{
  return 42;
}

int main()
{
  return current_value() == 42 ? 0 : 1;
}
