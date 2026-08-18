template <typename T>
struct ProbeVector
{
  T value;
};

template <typename T>
struct ProbeList
{
  T item;
};

template <typename T>
struct ProbeBox
{
  typedef ProbeVector<T> VertexType;

  ProbeList<ProbeVector<T>> Items() const;
  ProbeList<VertexType> AliasItems() const;
};

template <typename T>
ProbeList<ProbeVector<T>> ProbeBox<T>::Items() const
{
  ProbeList<ProbeVector<T>> ret;
  return ret;
}

template <typename T>
ProbeList<typename ProbeBox<T>::VertexType> ProbeBox<T>::AliasItems() const
{
  ProbeList<VertexType> ret;
  return ret;
}

int main()
{
  ProbeBox<float> box;
  (void)box;
  return 0;
}
