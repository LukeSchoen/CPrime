struct Item
{
  int number;

  Item() : number(0) {}
  Item(const Item &other) : number(other.number) {}
};

template<typename T>
struct List
{
  T value;

  List() : value() {}
  List(const List &other);

  List &operator=(const List &)
  {
    value.number = -1;
    return *this;
  }
};

template<typename T>
List<T>::List(const List<T> &other) : value(other.value)
{
  ++value.number;
}

struct Owner
{
  List<Item> member;

  Owner() = default;
  Owner(const Owner &) = default;
};

int main()
{
  Owner source;
  source.member.value.number = 73;
  Owner copied(source);
  return copied.member.value.number == 74 ? 0 : 1;
}

// EXPECT_EXIT: 0
