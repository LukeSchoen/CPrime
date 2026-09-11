void unreachable();

int main()
{
  int values[2];
  if (&values[0] == &values[1])
    unreachable();
  if (&values[0] != &values[1])
    return 0;
  return 1;
}
