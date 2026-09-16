struct Base {};

struct Y
{
  template<class T> Y(T);
};

template<>
Y::Y<int>(int) {}

struct Bar : virtual Base
{
  template<class T> Bar(T const &cast);
};

template<>
Bar::Bar(int const &) {}

int main()
{
  Y y(1);
  Bar bar(1);
  return 0;
}
