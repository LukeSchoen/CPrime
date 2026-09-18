// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: const_value_init.  A namespace-scope const object of a class
// type with no user-provided constructor is value-initialized by `T()`.  That
// is a constant initializer, so it must not be rejected as a non-constant
// initializer element or lowered to a dynamic initializer.
struct ordered_range_t
{
};

struct ordered_unique_range_t : ordered_range_t
{
};

static const ordered_range_t ordered_range = ordered_range_t();
static const ordered_unique_range_t ordered_unique_range = ordered_unique_range_t();

int main()
{
  return 0;
}
