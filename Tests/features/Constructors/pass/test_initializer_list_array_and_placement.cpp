#include <initializer_list>
#include <new>

static int constructorCalls;
static int destructorCalls;

struct List {
  int sum;
  List(std::initializer_list<int> values) : sum(0) {
    ++constructorCalls;
    for (const int *p = values.begin(); p != values.end(); ++p)
      sum += *p;
  }
  ~List() {
    int begin = sum;
    if (begin >= 0) ++destructorCalls;
  }
};

List globals[] = {{1, 2}, {3, 4, 5}, {}};

int main() {
  if (constructorCalls != 3 || globals[0].sum != 3
      || globals[1].sum != 12 || globals[2].sum != 0)
    return 1;
  {
    List local({6, 7});
    if (local.sum != 13 || constructorCalls != 4) return 2;
  }
  if (destructorCalls != 1) return 3;
  union Storage { char bytes[sizeof(List)]; long long alignment; } storage;
  List *placed = new (storage.bytes) List({8, 9, 10});
  if (placed->sum != 27 || constructorCalls != 5) return 4;
  placed->~List();
  return destructorCalls != 2;
}
