/* A name lookup has to see the members of every namespace nominated by the
   using-directives in scope, not only the one the single-token lookup
   happened to report: an unqualified call forms one overload set from all of
   them, a global-qualified name reaches a nominated member, and a qualified
   name reaches what its namespace inherits through its own directives. */

namespace first
{
  int f() { return 1; }
  int f(double);
}

namespace second
{
  int f(int) { return 2; }
  int f(double);
}

int f(int, int) { return 3; }

using namespace first;
using namespace second;

/* A name that no enclosing scope declares is reachable only through the
   directives; lookup must still gather every nominated member. */
namespace third
{
  int g() { return 4; }
}

namespace fourth
{
  int g(int) { return 5; }
}

using namespace third;
using namespace fourth;

namespace sized
{
  namespace inner
  {
    char (*size(int value))[42] { return 0; }
  }
  using namespace inner;
}

using namespace sized;

char probe[sizeof *::size(0) == 42 ? 1 : -1];

namespace valued
{
  namespace inner { int v = 6; }
  using namespace inner;
}

using namespace valued;

int direct = ::v;

namespace nested
{
  namespace inner { int i = 5; }
  using namespace inner;
}

namespace alias = nested;

int main()
{
  if (f() != 1)
    return 1;
  if (f(1) != 2)
    return 2;
  if (f(0, 0) != 3)
    return 3;
  if (g() != 4)
    return 4;
  if (g(1) != 5)
    return 5;
  if (alias::i != 5)
    return 6;
  if (sizeof probe != 1)
    return 7;
  if (direct != 6)
    return 8;
  return 0;
}
