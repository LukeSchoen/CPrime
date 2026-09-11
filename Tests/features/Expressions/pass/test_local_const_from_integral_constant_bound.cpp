// A const local initialized from another integral constant is a constant
// expression: it can bound an initialized array, and it is usable as a
// template argument, at function scope and in template member bodies.

template<int Count> struct Sized {
  int data[Count];
};

int constant_bound ()
{
  const int e (2);
  const int c (e);
  Sized<c> probe;
  int d[c] = { 0, 0 };
  return d[0] + (int) sizeof (d) + (int) sizeof (probe);
}

struct Owner {
  static int constant_bound ()
  {
    const int e (2);
    const int c (e);
    int d[c] = { 0, 0 };
    return d[0] + (int) sizeof (d);
  }
};

template<int> struct Wrapper {
  static int constant_bound ()
  {
    const int e (2);
    const int c (e);
    int d[c] = { 0, 0 };
    return d[0] + (int) sizeof (d);
  }
};

int main ()
{
  const int element = (int) sizeof (int);
  if (constant_bound () != 4 * element)
    return 1;
  if (Owner::constant_bound () != 2 * element)
    return 2;
  if (Wrapper<1>::constant_bound () != 2 * element)
    return 3;
  return 0;
}
