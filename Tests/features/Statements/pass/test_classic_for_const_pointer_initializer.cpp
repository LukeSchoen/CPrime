int main()
{
  int values[3] = { 4, 5, 6 };
  const int* end = values + 3;
  int sum = 0;
  for (const int* value = values; value != end; ++value)
    sum += *value;
  return sum == 15 ? 0 : 1;
}
