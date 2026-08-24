template<typename T>
T minimum_value(const T &lhs, const T &rhs)
{
  return lhs < rhs ? lhs : rhs;
}

template<typename T>
void move_values(T *target, T *source, long count)
{
  for (long i = 0; i < count; ++i)
    target[i] = source[i];
}

template<typename T>
struct ReplayList
{
  T *data;
  long size;

  ReplayList() : data((T *)0), size(0) {}
  T *Data();
  bool Reallocate(long capacity);
};

template<typename T>
T *ReplayList<T>::Data()
{
  return data;
}

template<typename T>
bool ReplayList<T>::Reallocate(long capacity)
{
  T *new_data = (T *)0;
  if (data)
    move_values(new_data, Data(), minimum_value(size, capacity));
  return true;
}

struct Pair
{
  int key;
  float value;
  Pair() : key(int()), value(float()) {}
};

int main()
{
  ReplayList<Pair> values;
  return values.Reallocate(1) ? 0 : 1;
}
