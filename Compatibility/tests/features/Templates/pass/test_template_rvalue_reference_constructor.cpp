// EXPECT_EXIT: 0

template<typename T>
class Box
{
public:
  T value;

  Box(T seed)
  {
    value = seed;
  }

  Box(Box<T> &&other)
  {
    value = other.value + 5;
  }
};

int main(void)
{
  Box<int> original(8);
  Box<int> moved(static_cast<Box<int>&&>(original));
  return moved.value == 13 ? 0 : 1;
}
