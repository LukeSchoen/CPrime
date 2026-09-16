// [over.ics.rank]/4.4 and /4.5.1: converting a derived pointer to a base
// pointer beats converting it to void*, and a nearer base beats a farther one.
// Reduced from boost/exception/exception.hpp, where
// copy_boost_exception(exception*, exception const*) competes with the
// copy_boost_exception(void*, void const*) fallback for a clone_impl<T>*.
struct A {};
struct B : A {};
struct C : B {};

int choose(A *) { return 1; }
int choose(B *) { return 2; }
int choose(void *) { return 3; }

int main()
{
  C *derived = 0;
  if (choose(derived) != 2) return 1;
  B *middle = 0;
  if (choose(middle) != 2) return 2;
  A *base = 0;
  if (choose(base) != 1) return 3;
  int *unrelated = 0;
  if (choose(unrelated) != 3) return 4;
  return 0;
}
