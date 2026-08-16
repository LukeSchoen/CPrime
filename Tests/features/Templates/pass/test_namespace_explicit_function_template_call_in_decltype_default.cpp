namespace ns
{
  template<typename T>
  T declval();
}

class NamespaceDeclvalDefault
{
public:
  template<typename T, typename = decltype(ns::declval<T>())>
  explicit NamespaceDeclvalDefault(const T &value)
  {
  }
};

template<typename T, typename = decltype(ns::declval<T>())>
NamespaceDeclvalDefault MakeNamespaceDeclvalDefault(const T &value)
{
  return NamespaceDeclvalDefault(value);
}

int main()
{
  return 0;
}
