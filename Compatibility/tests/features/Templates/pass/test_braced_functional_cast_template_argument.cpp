// EXPECT_EXIT: 0
#include <cstddef>

template<int N> struct Constant
{
  static const int value = N;
};

template<std::size_t N> struct Array
{
  typedef int type[N];
};

template<class N> using Repeat = typename Array<std::size_t{ N::value }>::type;
template<class N> using RepeatParen = typename Array<(std::size_t{ N::value })>::type;

int main()
{
  Repeat<Constant<3> > array;
  RepeatParen<Constant<2> > paren_array;
  array[0] = 1;
  paren_array[0] = 1;
  return sizeof(array) == 3 * sizeof(int)
      && sizeof(paren_array) == 2 * sizeof(int) ? 0 : 1;
}
