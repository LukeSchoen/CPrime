// EXPECT_EXIT: 0

template<typename T>
class X
{
public:
  template<typename K>
  T* find(const K& key);
  int value;
};

int main(void)
{
  X<int> x;
  x.value = 9;
  return x.value == 9 ? 0 : 1;
}
