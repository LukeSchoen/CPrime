// EXPECT_EXIT: 0

template<typename T>
int consume(T &&value)
{
  return value;
}

int main(void)
{
  return consume(42) == 42 ? 0 : 1;
}
