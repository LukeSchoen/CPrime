// A class-template specialization spelled with a reference or pointer in a
// declaration keeps only its identity until the declaration is parsed, so a
// member operator reached through that parameter must materialize the
// specialization before the candidate lookup.

template <class T>
struct Holder
{
  T value;

  Holder (T v) : value (v) {}
  int operator+ (int rhs) const { return value + rhs; }

  template <class U>
  bool operator== (const Holder<U> &rhs) const { return value == rhs.value; }
};

int add (const Holder<int> &lhs)
{
  return lhs + 1;
}

int deref (const Holder<int> *lhs)
{
  return *lhs + 1;
}

bool equal (const Holder<int> &lhs, const Holder<int> &rhs)
{
  return lhs == rhs;
}

int main ()
{
  Holder<int> value (41);
  Holder<int> same (41);
  Holder<int> other (7);

  if (add (value) != 42)
    return 1;
  if (deref (&value) != 42)
    return 2;
  if (!equal (value, same))
    return 3;
  if (equal (value, other))
    return 4;
  return 0;
}
