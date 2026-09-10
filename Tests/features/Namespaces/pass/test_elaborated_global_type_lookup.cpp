// EXPECT_EXIT: 0
namespace A
{
}

namespace N
{
  struct A;
}

using namespace N;

struct ::A *global_a;

namespace Q
{
  namespace Inner
  {
    struct S;
  }

  using namespace Inner;

  struct ::A *qualified_a;
}

int main()
{
  return global_a || Q::qualified_a;
}
