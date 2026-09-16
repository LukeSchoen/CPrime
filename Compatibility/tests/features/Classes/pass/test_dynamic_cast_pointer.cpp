struct Item { virtual int kind() const { return 1; } };
struct Container : Item { int kind() const override { return 2; } };
struct Other : Item {};
struct Left { virtual int left() { return 3; } };
struct Right { virtual int right() { return 4; } };
struct Multiple : Left, Right {};
struct Target { int value; };
struct L : Target {};
struct R : Target {};
struct Ambiguous : Item, L, R {};
struct Hidden : Item, private Target {};
struct Branch : Item {};
struct PrivateOuter : private Branch {
  Item *source() { return this; }
};
namespace Alpha { struct Thing : Item {}; }
namespace Beta { struct Thing : Item {}; }
template<class T> struct Typed : Item {};
int calls;
Item *once(Item *p) { ++calls; return p; }
int main()
{
  Item plain; Container container; Other other;
  Item *items[] = {&plain, &container, &other, 0};
  if (dynamic_cast<Container *>(once(items[0])) != 0 || calls != 1) return 1;
  if (dynamic_cast<Container *>(items[1]) != &container) return 2;
  if (dynamic_cast<Container *>(items[2]) || dynamic_cast<Container *>(items[3])) return 3;
  const Item *constant = &container;
  if (dynamic_cast<const Container *>(constant) != &container) return 4;
  Multiple multiple; Right *right = &multiple; Left *left = &multiple;
  if (dynamic_cast<Multiple *>(right) != &multiple) return 5;
  if (dynamic_cast<Left *>(right) != left || dynamic_cast<Right *>(left) != right) return 6;
  if (dynamic_cast<void *>(right) != &multiple) return 7;
  if (dynamic_cast<const void *>(static_cast<const Right *>(right)) != &multiple) return 8;
  Ambiguous ambiguous; Item *a = &ambiguous;
  if (dynamic_cast<Target *>(a)) return 9;
  Hidden hidden; Item *h = &hidden;
  if (dynamic_cast<Target *>(h)) return 10;
  PrivateOuter outer; Item *s = outer.source();
  if (!dynamic_cast<Branch *>(s) || dynamic_cast<PrivateOuter *>(s)) return 11;
  Alpha::Thing alpha; Item *named = &alpha;
  if (dynamic_cast<Beta::Thing *>(named) || dynamic_cast<Alpha::Thing *>(named) != &alpha) return 12;
  Typed<int> typed; Item *generic = &typed;
  if (dynamic_cast<Typed<long> *>(generic) || dynamic_cast<Typed<int> *>(generic) != &typed) return 13;
  return 0;
}
