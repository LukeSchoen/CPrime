int main(void)
{
  int values[3] = {1, 2, 3};
  int sum = 0;
  for (auto &value : values)
    sum += value;
  return sum - 6;
}
