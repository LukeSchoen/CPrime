int LocalPrototypeTarget(int value)
{
  return value + 1;
}

int main()
{
  int answer(41);
  const char* text("tinyxml2");
  int LocalPrototypeTarget(int value);

  return answer == 41 && text[0] == 't'
             && LocalPrototypeTarget(answer) == 42
           ? 0 : 1;
}
