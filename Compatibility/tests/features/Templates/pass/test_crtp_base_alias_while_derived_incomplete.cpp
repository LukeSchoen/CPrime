template<class Derived>
struct Base
{
  using size_type = unsigned;

  Derived& Self(Derived& value) { return value; }
};

template<class Value>
struct Derived : Base<Derived<Value> >
{
  using base_type = Base<Derived<Value> >;
  using size_type = typename base_type::size_type;

  size_type Size() const { return 9; }
};

int main()
{
  Derived<int> value;
  return value.Size() == 9 ? 0 : 1;
}
