// EXPECT_EXIT: 0
// A function template-id is a valid template argument naming the
// specialization, both for a non-type parameter of function type and in an
// explicit member-template call.
template <typename T> void default_initialize(const T &) {}

template <typename T> int twice(T value) { return 2 * value; }

template <typename OBJECT, void init_function(const OBJECT &)> struct Container {
  template <typename INITIALIZER> void Add(const INITIALIZER &initializer) {
    init_function(initializer);
  }
};

template <int (*function)(int)> struct Table {
  static int apply(int value) { return function(value); }
};

struct Holder {
  template <int (*function)(int)> int set(int value) { return function(value); }
};

static int applied;
static int record(int value) {
  applied += value;
  return value;
}

int main() {
  Container<int, default_initialize<int> > container;
  container.Add(42);
  if (Table<twice<int> >::apply(21) != 42)
    return 1;
  Holder holder;
  if (holder.set<twice<int> >(3) != 6)
    return 2;
  applied = 0;
  if (holder.set<record>(3) != 3)
    return 3;
  return applied == 3 ? 0 : 4;
}
