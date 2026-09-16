template <typename T>
struct NestedDeclBox
{
  T value;

  void PushBack(const T &next)
  {
    value = next;
  }
};

struct NestedDeclOuter
{
  struct Item
  {
    int number;
  };
};

int main()
{
  NestedDeclBox<NestedDeclOuter::Item> box;
  NestedDeclOuter::Item item;
  item.number = 21;
  box.value = item;
  box.PushBack(item);
  NestedDeclBox<NestedDeclOuter::Item> *pbox = &box;
  pbox->PushBack(item);
  return box.value.number != 21;
}
