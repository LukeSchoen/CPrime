/* An elaborated-type-specifier with a dependent nested-name-specifier names
   the nested tag: the data member `int Item` must not hide `struct Item`, the
   enumerator `Pick` of `enum Alias` must not hide `enum Pick`, and the union
   tag must be reachable through the dependent qualifier too. */
struct Owner {
  struct Item { int value; };
  int Item;
  union Part { int m; };
};
struct Choice {
  enum Pick { first = 1 };
  enum Alias { Pick = 2 };
};

template<class T, class U> int probe() {
  struct T::Item item;
  enum U::Pick pick;
  union T::Part part;
  item.value = 3;
  part.m = 4;
  (void)pick;
  return item.value + part.m;
}

int main() { return probe<Owner, Choice>() == 7 ? 0 : 1; }
