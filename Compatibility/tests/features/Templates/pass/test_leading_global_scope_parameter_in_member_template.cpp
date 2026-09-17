/* An out-of-class member function template whose parameter type is spelled
   with an explicit global scope operator.

   The leading `::` reaches the surrogate `(::type name)` argument list the
   replay builds for an inline member template; the classifier read it as a
   direct initializer expression and dropped the whole declarator, so the
   definition was compared against the declaration as `inline int` versus
   `int (unsigned long long)`. */

typedef unsigned long long size_like;

namespace outer
{
  typedef unsigned long long size_like;
}

struct Holder
{
  template <class U>
  static int convert(::size_like value) { return (int)value; }

  template <class U>
  static int twice(int first, ::outer::size_like second)
  {
    return first + (int)second;
  }
};

template <class U>
int free_convert(::size_like value)
{
  return (int)value + 1;
}

int main()
{
  if (Holder::convert<int>(3) != 3)
    return 1;
  if (Holder::twice<int>(1, 3) != 4)
    return 2;
  if (free_convert<int>(3) != 4)
    return 3;
  return 0;
}
