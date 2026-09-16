template<typename T>
struct VariadicMemberBox
{
  T value;

  template<typename... Args>
  void EmplaceAt(int index, Args... args);

  template<typename... Args>
  void EmplaceBack(Args... args);
};

template<typename T>
template<typename... Args>
void VariadicMemberBox<T>::EmplaceAt(int index, Args... args)
{
  (void)index;
  value = T(args...);
}

template<typename T>
template<typename... Args>
void VariadicMemberBox<T>::EmplaceBack(Args... args)
{
  EmplaceAt(0, args...);
}

struct VariadicPackA
{
};

struct VariadicPackB
{
};

struct VariadicMemberValue
{
  int first;
  int second;

  VariadicMemberValue()
    : first(0), second(0)
  {
  }

  VariadicMemberValue(int a, VariadicPackA b)
    : first(a), second(11)
  {
    (void)b;
  }

  VariadicMemberValue(int a, VariadicPackB b)
    : first(a), second(103)
  {
    (void)b;
  }
};

int main()
{
  VariadicMemberBox<VariadicMemberValue> box;
  VariadicPackA a;
  VariadicPackB b;
  box.EmplaceBack(7, a);
  if (box.value.first != 7 || box.value.second != 11)
    return 1;
  box.EmplaceBack(9, b);
  return box.value.first == 9 && box.value.second == 103 ? 0 : 2;
}

// EXPECT_EXIT: 0
