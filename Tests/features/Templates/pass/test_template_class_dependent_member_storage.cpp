// EXPECT_EXIT: 0

template<typename T>
class X
{
private:
  T data;
  T* ptr;

public:
  void set(T value)
  {
    this->data = value;
    this->ptr = &this->data;
  }

  T get()
  {
    return *this->ptr;
  }
};

int main(void)
{
  X<int> x;
  x.set(6);
  return x.get() == 6 ? 0 : 1;
}
