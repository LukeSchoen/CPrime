// EXPECT_EXIT: 0

struct RangeItem
{
  int value;
};

struct RangeItems
{
  RangeItem data[2];

  int Size() { return 2; }
  RangeItem &operator[](int index) { return data[index]; }
};

int main(void)
{
  RangeItems items;
  items.data[0].value = 1;
  items.data[1].value = 2;

  int sum = 0;
  for (auto &item : items)
  {
    sum += item.value;
  }
  for (int i = 0; i < items.Size(); i++)
  {
    sum += i;
  }

  return sum == 4 ? 0 : 1;
}
