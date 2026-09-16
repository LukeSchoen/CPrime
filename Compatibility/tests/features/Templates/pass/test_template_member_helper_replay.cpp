typedef long long i64;

template<typename T>
void MoveConstruct(T *target, T *source, const i64 count = 1)
{
  for (i64 i = 0; i < count; ++i)
    target[i] = source[i];
}

template<typename T>
void Destruct(T *target, const i64 count = 1)
{
  (void)target;
  (void)count;
}

template<typename T>
void Construct(T *target, const T &value)
{
  *target = value;
}

template<typename T>
class ReplayList
{
public:
  i64 m_size;
  T m_data[8];

  T *Data();
  void Grow(i64 requiredCapacity);
  void Move(i64 srcIndex, i64 dstIndex, i64 count);
  void Insert(i64 index, T &&value);
  void Insert(i64 index, const T &value);
};

template<typename T>
T *ReplayList<T>::Data()
{
  return m_data;
}

template<typename T>
void ReplayList<T>::Grow(i64 requiredCapacity)
{
  if (requiredCapacity > 8)
    requiredCapacity = 8;
}

template<typename T>
void ReplayList<T>::Move(i64 srcIndex, i64 dstIndex, i64 count)
{
  MoveConstruct(Data() + dstIndex, Data() + srcIndex, count);
  MoveConstruct(Data() + dstIndex, Data() + srcIndex);
  Destruct(Data() + srcIndex);
}

template<typename T>
void ReplayList<T>::Insert(i64 index, T &&value)
{
  Grow(m_size + 1);
  Move(index, index + 1, m_size - index);
  Construct(Data() + index, value);
  ++m_size;
}

template<typename T>
void ReplayList<T>::Insert(i64 index, const T &value)
{
  Insert(index, (T)value);
}

int main()
{
  ReplayList<int> list;
  list.m_size = 0;
  list.Insert(0, 7);
  if (list.m_data[0] != 7 || list.m_size != 1)
    return 1;
  return 0;
}
