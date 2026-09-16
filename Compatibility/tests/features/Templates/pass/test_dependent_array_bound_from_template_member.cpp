// An array typedef inside a class template needs its bound as a value.  The
// eager alias parse ran while the class alias layout depth was set, which
// forced a referenced class-template specialization to stay an incomplete
// forward declaration, so a bound such as `flags<A>::enabled` could not find
// the static member and reported it as undeclared.  The declaration is now
// deferred to the ordinary class replay where the specialization is
// materialized.

template <typename T> struct flags {
  static const bool enabled = true;
};

template <typename A> struct holder {
  typedef char buffer[flags<A>::enabled ? 4 : 1];
  buffer storage;
};

template <typename A> struct empty {
  typedef char buffer[flags<A>::enabled ? 1 : 5];
  buffer storage;
};

int main() {
  holder<int> h;
  empty<int> e;
  if (sizeof(h.storage) != 4) return 1;
  if (sizeof(e.storage) != 1) return 2;
  return 0;
}
