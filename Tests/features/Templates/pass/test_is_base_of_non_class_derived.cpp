/* `__is_base_of` answers false for a non-class derived type without
   materializing its first argument, so a class template that cannot be
   instantiated is still usable as the base argument (PR c++/50732). */

template <typename T>
struct non_instantiable
{
  typedef typename T::THIS_TYPE_CANNOT_BE_INSTANTIATED type;
};

int check[__is_base_of (non_instantiable<int>, void) ? -1 : 1];

struct Base { };
struct Derived : Base { };

int real_check[__is_base_of (Base, Derived) ? 1 : -1];

int main ()
{
  if (sizeof (check) != sizeof (int))
    return 1;
  if (sizeof (real_check) != sizeof (int))
    return 2;
  return 0;
}
