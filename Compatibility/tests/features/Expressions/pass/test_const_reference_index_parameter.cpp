int indexValue(const int *values, const long long &index)
{
  return values[index];
}

int main()
{
  int values[2] = { 7, 9 };
  long long index = 1;
  if (indexValue(values, index) != 9)
    return 1;
  return indexValue(values, 0) == 7 ? 0 : 2;
}

// EXPECT_EXIT: 0
