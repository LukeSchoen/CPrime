template <typename> struct Base;
template <typename Type = void> struct Value : Base<Type>::Alias {
  typedef typename Base<Type>::Alias Parent;
  Value() : Parent() {}
};

template <> struct Base<void> { typedef Value<> Alias; };
template <> struct Value<> { int number; Value() : number(23) {} };

int main()
{
  Base<void>::Alias value;
  return value.number != 23;
}
