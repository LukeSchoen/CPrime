// EXPECT_EXIT: 0
class Text
{
  int selected;

  void Split(char separator)
  {
    selected = 1;
  }

  void Split(const char *separator)
  {
    selected = 2;
  }
};

int main(void)
{
  Text text;
  char lineSeparator = '\n';

  text.selected = 0;
  text.Split(lineSeparator);
  if (text.selected != 1)
    return 1;

  text.selected = 0;
  text.Split('\n');
  if (text.selected != 1)
    return 2;

  text.selected = 0;
  text.Split("\n");
  if (text.selected != 2)
    return 3;

  return 0;
}
