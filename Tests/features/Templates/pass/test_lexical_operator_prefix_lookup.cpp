bool operatorY()
{
  return false;
}

struct OperatorPrefixLookup
{
  int value;

  OperatorPrefixLookup() : value(0)
  {
    while (operatorY())
      ++value;
  }
};

int main()
{
  OperatorPrefixLookup value;
  return value.value;
}

// EXPECT_EXIT: 0
