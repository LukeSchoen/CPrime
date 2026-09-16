namespace Shapes {
struct Named { int value; };
int named(Named const& item) { return item.value; }
typedef struct { int value; } Item;
int read(Item const& item) { return item.value; }
int rank(Item const&, int) { return 2; }
int blocked(Item const&, int) { return 2; }
int pointed(Item const* item) { return item->value; }
template<class T> int templated(T const& item) { return item.value; }
}
int rank(Shapes::Item const&, long) { return 1; }
int read(...) { return 9; }
int blocked(Shapes::Item const&, long) { return 1; }
struct Local { int operator()(Shapes::Item const&) { return 7; } };
int main() {
 Shapes::Item item = { 4 };
 Shapes::Named other = { 6 };
 if (named(other) != 6) return 5;
 {
   extern int blocked(Shapes::Item const&, long);
   if (blocked(item, 0) != 1) return 6;
 }
 if (read(item) != 4 || rank(item, 0) != 2) return 1;
 if (pointed(&item) != 4 || templated(item) != 4) return 2;
 if (::read(item) != 9 || ::rank(item, 0) != 1) return 3;
 Local read;
 if (read(item) != 7) return 4;
 return 0;
}
