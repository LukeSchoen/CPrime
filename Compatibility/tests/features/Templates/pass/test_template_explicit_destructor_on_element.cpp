// EXPECT_EXIT: 0
#include <new>

int destroyed;

class Item
{
public:
  int value;

  Item() : value(0) {}

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
    new (&this->data[0]) T();
    this->data[1].~T();
    new (&this->data[1]) T();
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
  // The by-value fill arguments have already been destroyed.
  destroyed = 0;
  store.clear();
  return destroyed == 7 ? 0 : 1;
}
