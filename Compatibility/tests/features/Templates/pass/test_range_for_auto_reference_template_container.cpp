template <typename T>
struct TemplateItems
{
  T data[2];

  int Size() { return 2; }
  T &operator[](int index) { return data[index]; }
};

int main()
{
  TemplateItems<int> items;
  items.data[0] = 1;
  items.data[1] = 2;
  int sum = 0;
  for (auto &piece : items)
    sum += piece;
  return sum != 3;
}
