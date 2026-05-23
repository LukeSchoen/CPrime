template<typename T>
class cpcList
{
public:
  T *items;
  int count;

  cpcList()
  {
    this->items = (T *)0;
    this->count = 0;
  }

  ~cpcList()
  {
    this->count = 0;
  }
};

template<typename T>
class Holder
{
  cpcList<T> items;
  int count;

  int size()
  {
    return this->count;
  }
};

int main(void)
{
  Holder<int> h;
  h.count = 7;
  return h.size() == 0 ? 0 : 1;
}
