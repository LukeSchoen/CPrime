// EXPECT_EXIT: 0

template<typename T>
class Holder
{
public:
  T value;

  explicit Holder(T seed)
  {
    value = seed + 2;
  }
};

int main(void)
{
  Holder<int> holder(5);
  return holder.value == 7 ? 0 : 1;
}
