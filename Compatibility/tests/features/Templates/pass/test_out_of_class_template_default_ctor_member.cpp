// EXPECT_EXIT: 0

template<typename T>
struct Box
{
  T value;
  Box();
};

template<typename T>
Box<T>::Box()
{
  value = 7;
}

struct Holder
{
  Box<int> box;
};

int main()
{
  Holder holder;
  return holder.box.value == 7 ? 0 : 1;
}
