inline int RepeatedInlineAcrossInputs(int value)
{
  return value + 1;
}

int RepeatedInlineFromA();

int main()
{
  return RepeatedInlineFromA() != 3 || RepeatedInlineAcrossInputs(4) != 5;
}
