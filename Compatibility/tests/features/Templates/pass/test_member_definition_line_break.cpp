// An out-of-class member definition may put its qualified declarator-id on
// the line after `Class<T>::`.  Both overloads below remain class-template
// members, and the call must select the definition for its parameter type.

template <class R, class T>
struct Pair
{
};

template <class T>
struct Owner
{
  int selected;

  Owner (int seed);
  void select (Pair<char, T> const &);
  void select (Pair<T, T> const &);
  static int base ();
};

template <class T>
Owner<T>::
Owner (int seed) : selected(seed)
{
}

template <class T>
void Owner<T>::
select (Pair<char, T> const &)
{
  selected = 1;
}

template <class T>
void Owner<T>::
select (Pair<T, T> const &)
{
  selected = 2;
}

template <class T>
int Owner<T>::
base ()
{
  return 7;
}

int main ()
{
  Owner<int> owner(0);
  Pair<char, int> first;
  Pair<int, int> second;

  if (Owner<int>::base() != 7)
    return 1;
  owner.select(first);
  if (owner.selected != 1)
    return 2;
  owner.select(second);
  if (owner.selected != 2)
    return 3;
  return 0;
}
