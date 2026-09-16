// EXPECT_EXIT: 0

int main()
{
  int *values = new int[4];
  if (!values)
    return 1;
  values[2] = 17;
  int result = values[2] == 17 ? 0 : 2;
  delete[] values;
  return result;
}
