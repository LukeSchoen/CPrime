// EXPECT_EXIT: 0

int destroyed;

class Item
{
public:
  int value;

  ~Item()
  {
    destroyed = destroyed + this->value;
  }
};

template<typename T>
class Store
{
private:
  T data[2];

public:
  void fill(T a, T b)
  {
    this->data[0] = a;
    this->data[1] = b;
  }

  void clear()
  {
    this->data[0].~T();
    this->data[1].~T();
  }
};

int main(void)
{
  Store<Item> store;
  Item a;
  Item b;

  destroyed = 0;
  a.value = 3;
  b.value = 4;
  store.fill(a, b);
  store.clear();
  return destroyed == 7 ? 0 : 1;
}
