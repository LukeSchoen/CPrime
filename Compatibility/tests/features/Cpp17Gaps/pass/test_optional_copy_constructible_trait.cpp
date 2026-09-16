// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: std_optional_copy_constructible. An optional is copy
// constructible only when its element type is.
#include <optional>
#include <type_traits>

struct MoveOnly {
  MoveOnly() {}
  MoveOnly(const MoveOnly &) = delete;
  MoveOnly &operator=(const MoveOnly &) = delete;
  MoveOnly(MoveOnly &&) {}
  MoveOnly &operator=(MoveOnly &&) { return *this; }
};

static_assert(std::is_copy_constructible<std::optional<int> >::value,
              "optional<int> is copy constructible");
static_assert(!std::is_copy_constructible<std::optional<MoveOnly> >::value,
              "optional of a move-only type is not copy constructible");
static_assert(std::is_move_constructible<std::optional<MoveOnly> >::value,
              "optional of a move-only type is move constructible");

int main() { return 0; }
