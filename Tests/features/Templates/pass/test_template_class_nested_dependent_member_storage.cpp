// EXPECT_EXIT: 0

template<typename T>
class Vec
{
public:
  T data;
};

template<typename T>
class Holder
{
private:
  Vec<T> inner;

public:
  void set(T value)
  {
    this->inner.data = value;
  }

  T get()
  {
    return this->inner.data;
  }
};

int main(void)
{
  Holder<int> h;
  h.set(9);
  return h.get() == 9 ? 0 : 1;
}
