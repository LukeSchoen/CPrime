template<typename T>
struct DelegatingBox
{
  long long size;
  long long capacity;
  T *data;

  DelegatingBox() : size(0), capacity(0), data(0) {}

  DelegatingBox(int value) : DelegatingBox<T>()
  {
    if (size != 0 || capacity != 0 || data != 0)
      size = -1;
    else
      size = value;
  }
};

int main()
{
  DelegatingBox<int> box(73);
  return box.size == 73 && box.capacity == 0 && box.data == 0 ? 0 : 1;
}

// EXPECT_EXIT: 0
