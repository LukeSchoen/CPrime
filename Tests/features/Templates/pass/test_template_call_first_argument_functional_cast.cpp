// EXPECT_EXIT: 0

template<typename T>
T larger(T a, T b)
{
  return a > b ? a : b;
}

int main()
{
  return larger(int(3.5), 1) == 3 ? 0 : 1;
}
