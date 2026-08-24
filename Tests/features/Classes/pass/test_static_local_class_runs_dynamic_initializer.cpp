struct StaticLocalValue
{
  int value;

  StaticLocalValue(const char *text)
  {
    value = text[0];
  }
};

int readStaticLocalValue()
{
  static StaticLocalValue value = "Q";
  return value.value;
}

int main()
{
  return readStaticLocalValue() == 'Q' && readStaticLocalValue() == 'Q'
           ? 0 : 1;
}

// EXPECT_EXIT: 0
