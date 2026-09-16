// EXPECT_EXIT: 0
// An explicit non-type argument on a static member template selected through
// an object-qualified class name must not turn the static call into a member
// call with an implicit receiver.
namespace tag {
enum value { zero };
}

int calls;

struct X {
  template <tag::value V> void member() { calls += 1; }
  template <tag::value V> static void statik() { calls += 1; }
};

template <class C>
void invoke(C *ptr) {
  ptr->X::member<tag::zero>();
  ptr->C::template member<tag::zero>();
  ptr->template member<tag::zero>();
  ptr->X::statik<tag::zero>();
  ptr->C::template statik<tag::zero>();
  ptr->template statik<tag::zero>();
  X::statik<tag::zero>();
  C::template statik<tag::zero>();
}

int main() {
  X x;
  invoke(&x);
  return calls != 8;
}
