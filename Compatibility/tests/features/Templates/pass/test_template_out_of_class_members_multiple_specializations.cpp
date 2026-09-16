template<typename T>
struct MultiReplayBox
{
  T value;
  void Set(const T &next);
  T Get() const;
};

template<typename T>
void MultiReplayBox<T>::Set(const T &next)
{
  value = next;
}

template<typename T>
T MultiReplayBox<T>::Get() const
{
  return value;
}

typedef MultiReplayBox<int> IntMultiReplayBox;
typedef MultiReplayBox<char> CharMultiReplayBox;

int main()
{
  IntMultiReplayBox ints;
  CharMultiReplayBox chars;
  int three = 3;
  char x = 'x';
  ints.Set(three);
  chars.Set(x);
  return ints.Get() == 3 && chars.Get() == 'x' ? 0 : 1;
}
