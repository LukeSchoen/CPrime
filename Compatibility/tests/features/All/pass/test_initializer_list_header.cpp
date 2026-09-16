// EXPECT_EXIT: 0

#include <initializer_list>

int count_values(const std::initializer_list<int> &values)
{
  return (int)values.size();
}

int main()
{
  std::initializer_list<int> values;
  return count_values(values);
}
