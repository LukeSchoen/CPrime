// EXPECT_EXIT: 0

template<typename T>
class Cell
{
public:
  T value;

  T get();
  T add(T other);
};

template<typename T>
T Cell<T>::get() { return this->value; }

template<typename T>
T Cell<T>::add(T other) { return this->value + other; }

int main(void)
{
  Cell<int> cell;
  cell.value = 10;
  return cell.get() == 10 && cell.add(32) == 42 ? 0 : 1;
}
