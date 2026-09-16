// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: std_is_copy_assignable.
#include <type_traits>

struct Copyable {
  Copyable &operator=(const Copyable &) { return *this; }
};

struct MoveOnly {
  MoveOnly &operator=(MoveOnly &&) { return *this; }
};

static_assert(std::is_copy_assignable<int>::value, "scalars are copy assignable");
static_assert(std::is_copy_assignable<Copyable>::value, "copy assignable");
static_assert(!std::is_copy_assignable<MoveOnly>::value, "move only");

int main() { return 0; }
