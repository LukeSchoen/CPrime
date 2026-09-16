// EXPECT_EXIT: 0

template<typename T>
class Box
{
public:
  T value;

  T operator[](int index);
};

template<typename T>
T Box<T>::operator[](int index)
{
  return this->value + index;
}

int main(void)
{
  Box<int> box;
  box.value = 39;
  return box[3] == 42 ? 0 : 1;
}
