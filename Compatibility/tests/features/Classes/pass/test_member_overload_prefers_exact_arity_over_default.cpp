class ExactDefaultChoice
{
public:
  int value;

  void Pick(int *values, int count);
  void Pick(int *values, int count, bool moveValues = false);
};

void ExactDefaultChoice::Pick(int *values, int count)
{
  value = 1;
}

void ExactDefaultChoice::Pick(int *values, int count, bool moveValues)
{
  value = 2;
}

int main()
{
  int values[1] = {0};
  ExactDefaultChoice choice;
  choice.value = 0;
  choice.Pick(values, 1);
  return choice.value == 1 ? 0 : 1;
}
