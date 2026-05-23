// EXPECT_EXIT: 0

template<typename T>
class Box
{
public:
  T value;
};

struct Entry
{
  Box<int> second;
};

int main(void)
{
  struct Entry e;
  e.second.value = 3;
  return e.second.value == 3 ? 0 : 1;
}
