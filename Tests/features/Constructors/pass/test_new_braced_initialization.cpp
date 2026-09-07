#include <new>
#include <memory>
#include <stdlib.h>
struct Item { int number; };
struct Ref { const int &key; Item *&value; };
struct Pair { int first; int second; };
struct Constructed { int value; Constructed(int a, int b) : value(a + b) {} };
int calls;
int next() { return ++calls; }
int main()
{
  int key = 7; Item item{42}; Item *value = &item;
  std::unique_ptr<Ref> cache;
  cache.reset(new Ref{key, value});
  if (&cache->key != &key || &cache->value != &value) return 6;
  Ref *r = new Ref{key, value};
  if (&r->key != &key || &r->value != &value || r->value->number != 42) return 1;
  Pair *p = new Pair{next(), next()};
  if (p->first != 1 || p->second != 2 || calls != 2) return 2;
  Constructed *c = new Constructed{3, 8};
  if (c->value != 11) return 3;
  int *scalar = new int{19};
  if (*scalar != 19) return 4;
  void *memory = malloc(sizeof(Ref));
  Ref *placed = new (memory) Ref{key, value};
  if (&placed->key != &key || &placed->value != &value) return 5;
  delete r; delete p; delete c; delete scalar; free(memory);
  return 0;
}
