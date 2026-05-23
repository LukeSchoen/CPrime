// EXPECT_EXIT: 0

template<typename T>
class Box
{
public:
  T get()
  {
    return value;
  }

  T value;
};

int main(void)
{
  Box<int> b;
  b.value = 42;
  return b.get() != 42;
}
