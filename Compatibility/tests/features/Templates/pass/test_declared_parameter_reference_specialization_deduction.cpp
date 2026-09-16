// An explicit specialization declaration deduces its arguments from the
// parameter types it spells. A bare `T` pattern matched with a reference
// parameter therefore deduces that reference type, so the friend declaration
// names the specialization whose parameter list it writes.
static int calls;

template <class T>
bool accepts(T) { ++calls; return true; }

struct Payload {
};

struct Owner {
  friend bool accepts<>(const Payload&);
};

template <class T>
struct Holder {
  friend bool accepts<>(const Holder&);
};

template struct Holder<int>;

int main()
{
  Payload payload;
  if (!accepts(payload) || calls != 1) return 1;
  return 0;
}
