template<typename T>
struct DelegatingCtorNoTemplateIdBox
{
  int value;
  DelegatingCtorNoTemplateIdBox();
  DelegatingCtorNoTemplateIdBox(const T &other);
};

template<typename T>
DelegatingCtorNoTemplateIdBox<T>::DelegatingCtorNoTemplateIdBox() : value(0)
{
}

template<typename T>
DelegatingCtorNoTemplateIdBox<T>::DelegatingCtorNoTemplateIdBox(const T &other)
  : DelegatingCtorNoTemplateIdBox()
{
  value = other;
}

typedef DelegatingCtorNoTemplateIdBox<int> IntDelegatingCtorNoTemplateIdBox;

int main()
{
  IntDelegatingCtorNoTemplateIdBox value(3);
  return value.value != 3;
}
