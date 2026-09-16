template <typename T>
struct NestedTypeHolder
{
  T value;
};

struct NestedTypeOwner
{
  struct Value
  {
    int number;
  };
};

int main()
{
  NestedTypeHolder<NestedTypeOwner::Value> holder;
  holder.value.number = 9;
  return holder.value.number != 9;
}
