template<typename T>
struct WorkList
{
  using Pointer = T *;

  T *data;
  int size;
  int capacity;

  WorkList() : data(0), size(0), capacity(0) {}
  T *Data();
  const T *Data() const;
  bool Realloc(int requested);
  bool TryGrow(int requested);
  bool TryReserve(int requested, bool exponential);
  static Pointer EmptyPointer();

  WorkList &operator=(const WorkList &rhs);

  template<long long N>
  WorkList &operator=(const T (&elements)[N]);
};

template<typename T>
typename WorkList<T>::Pointer WorkList<T>::EmptyPointer()
{
  return 0;
}

template<typename T>
T *WorkList<T>::Data()
{
  return data;
}

template<typename T>
const T *WorkList<T>::Data() const
{
  return data;
}

template<typename T>
bool WorkList<T>::Realloc(int requested)
{
  capacity = requested;
  return true;
}

template<typename T>
bool WorkList<T>::TryGrow(int requested)
{
  return TryReserve(requested, false);
}

template<typename T>
bool WorkList<T>::TryReserve(int requested, bool exponential)
{
  if (requested <= capacity)
    return true;
  return exponential ? TryGrow(requested) : Realloc(requested);
}

template<typename T>
WorkList<T> &WorkList<T>::operator=(const WorkList &rhs)
{
  if (this != &rhs)
  {
    data = rhs.data;
    size = rhs.size;
    capacity = rhs.capacity;
  }
  return *this;
}

template<typename T>
template<long long N>
WorkList<T> &WorkList<T>::operator=(const T (&elements)[N])
{
  data = (T *)elements;
  size = (int)N;
  capacity = (int)N;
  return *this;
}

struct DeferredValue;
WorkList<DeferredValue> make_deferred_list();

struct DeferredValue
{
  int value;
};

WorkList<DeferredValue> make_deferred_list()
{
  WorkList<DeferredValue> result;
  result.TryReserve(4, false);
  return result;
}

float scalar_root(float value)
{
  return value;
}

double scalar_root(double value)
{
  return value;
}

template<typename T>
T scalar_zero()
{
  return (T)0;
}

template<typename T>
struct clVector2
{
  T x;
  T y;

  const T &At(int index) const;
  auto Normalized() const;
  auto LengthSquared() const;
};

template<typename T>
clVector2<T> clCreateVector(const T &x, const T &y)
{
  clVector2<T> result;
  result.x = x;
  result.y = y;
  return result;
}

template<typename T>
const T &clVector2<T>::At(int index) const
{
  if (index)
    return y;
  return x;
}

template<typename T>
auto clVector2<T>::Normalized() const
{
  auto length = LengthSquared();
  if (length == scalar_zero<decltype(length)>())
    length = (decltype(length))1;
  return clCreateVector(x + length - length, y + length - length);
}

template<typename T>
auto clVector2<T>::LengthSquared() const
{
  return scalar_root(At(0) * At(0) + At(1) * At(1));
}

template<typename T>
auto normalize_vec2(const clVector2<T> &value)
{
  return value.Normalized();
}

struct ScalarReader
{
  template<typename T>
  bool Read(T &destination);

  template<typename T>
  bool Read(T *destination);

  template<typename T>
  long long Read(T *destination, long long count);
};

template<typename T>
bool ScalarReader::Read(T &destination)
{
  (void)destination;
  return false;
}

template<typename T>
bool ScalarReader::Read(T *destination)
{
  return Read(destination, 1) == 1;
}

template<typename T>
long long ScalarReader::Read(T *destination, long long count)
{
  (void)destination;
  return count;
}

struct ReaderOwner
{
  ScalarReader reader;
  bool ReadLocal();
};

bool ReaderOwner::ReadLocal()
{
  long long local = 0;
  return reader.Read(&local);
}

int main()
{
  WorkList<DeferredValue> first = make_deferred_list();
  WorkList<DeferredValue> second;
  clVector2<float> input;
  clVector2<float> output;
  ScalarReader reader;
  ReaderOwner owner;
  long long wide_value = 0;

  second = first;
  if (second.capacity != 4 || second.Data() != first.Data())
    return 1;
  if (WorkList<DeferredValue>::EmptyPointer() != first.Data())
    return 3;

  input.x = 3.0f;
  input.y = 7.0f;
  output = normalize_vec2(input);
  if (output.x != 3.0f || output.y != 7.0f)
    return 2;
  if (!reader.Read(&wide_value))
    return 4;
  if (!owner.ReadLocal())
    return 5;
  return 0;
}
