// EXPECT_EXIT: 0

template<typename T>
class Box
{
public:
  T value;

  Box(T v)
  {
    value = v;
  }

  Box(const Box<T>& other)
  {
    value = other.value;
  }
};

int main(void)
{
  Box<int> original(42);
  Box<int> copy(original);
  return copy.value == 42 ? 0 : 1;
}
