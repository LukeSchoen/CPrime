template <typename T>
struct Box
{
  enum { ElementCount = 2 };
  T values[ElementCount];
  typedef Box<T> BoxType;
};

template <typename T>
struct Pair
{
  enum { ElementCount = 3 };
  T values[ElementCount];
};

typedef Box<int> IntBox;
typedef Pair<int> IntPair;

int main()
{
  return IntBox::ElementCount == 2 && IntPair::ElementCount == 3 ? 0 : 1;
}
