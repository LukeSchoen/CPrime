template <unsigned Size, unsigned Alignment>
struct aligned_storage
{
  typedef char type[Size] __attribute__((aligned(Alignment)));
};

template <typename T>
struct storage
{
  typename aligned_storage<sizeof(T), __alignof(T)>::type value;
};

int main()
{
  return sizeof(storage<double>) != sizeof(double)
      || __alignof(storage<double>) != __alignof(double);
}
