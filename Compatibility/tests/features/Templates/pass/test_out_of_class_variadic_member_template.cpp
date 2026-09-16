// EXPECT_EXIT: 0

typedef long long i64;

template<typename T>
struct List
{
  i64 size;
  i64 capacity;
  T *data;

  List() : size(0), capacity(0), data(0) {}
  ~List() {}

  template<typename... Args>
  void Resize(i64 newSize, Args &&... args);
};

template<typename T>
template<typename... Args>
void List<T>::Resize(i64 newSize, Args &&... args)
{
  size = newSize;
}

int main()
{
  List<char> list;
  list.Resize(4, (char)'x');
  return list.size == 4 ? 0 : 1;
}
