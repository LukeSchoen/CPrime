int calls;
int first_value(int argument)
{
  static const int value = ++calls + argument;
  return value;
}
int attempts;
int initialize()
{
  if (++attempts < 2) throw 7;
  return 42;
}
int retry()
{
  static const int value = initialize();
  return value;
}
int constant_case(int value)
{
  static const int choice = 3;
  switch (value) { case choice: return 1; default: return 0; }
}
int main()
{
  if (first_value(5) != 6 || first_value(99) != 6 || calls != 1) return 1;
  try { retry(); return 2; } catch (int error) { if (error != 7) return 3; }
  if (retry() != 42 || retry() != 42 || attempts != 2) return 4;
  return constant_case(3) != 1 || constant_case(4) != 0;
}
