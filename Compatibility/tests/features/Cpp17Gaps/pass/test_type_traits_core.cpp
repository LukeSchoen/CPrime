// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: type_traits_core. <type_traits> carries the traits the runtime
// needs and little else: is_convertible, is_pointer, remove_pointer,
// remove_cv_t, common_type and most _v variable templates are absent.

#include <type_traits>

static_assert(std::is_convertible<int, double>::value, "is_convertible");
static_assert(!std::is_convertible<int, void *>::value, "is_convertible is implicit only");
static_assert(std::is_pointer<int *>::value, "is_pointer");
static_assert(!std::is_pointer<int &>::value, "is_pointer rejects references");
static_assert(std::is_arithmetic<int>::value, "is_arithmetic");
static_assert(std::is_same<std::remove_pointer<int *>::type, int>::value, "remove_pointer");
static_assert(std::is_same<std::remove_pointer_t<int *const>, int>::value, "remove_pointer_t");
static_assert(std::is_same<std::remove_cv_t<const int>, int>::value, "remove_cv_t");
static_assert(std::is_same<std::common_type<int, double>::type, double>::value, "common_type");
static_assert(std::is_same<std::common_type_t<int, double, float>, double>::value, "common_type_t");
static_assert(std::is_integral_v<int>, "is_integral_v");
static_assert(std::is_pointer_v<int *>, "is_pointer_v");
static_assert(!std::is_floating_point_v<int>, "is_floating_point_v");
static_assert(std::is_convertible_v<long, double>, "is_convertible_v");

// Trivial special members: scalars and pointers are trivial even though the
// legacy __has_trivial_destructor spelling answers false for them.
struct Plain
{
  int value;
};

struct WithVirtual
{
  virtual ~WithVirtual();
};

static_assert(std::is_trivially_destructible<int>::value, "int is trivially destructible");
static_assert(std::is_trivially_destructible<int *>::value, "pointer is trivially destructible");
static_assert(std::is_trivially_destructible<Plain>::value, "plain is trivially destructible");
static_assert(!std::is_trivially_destructible<WithVirtual>::value, "virtual destructor is not trivial");
static_assert(std::is_trivially_constructible<Plain, const Plain &>::value, "trivial copy construction");
static_assert(std::is_trivially_constructible<Plain, Plain &&>::value, "trivial move construction");
static_assert(std::is_trivially_copy_constructible<Plain>::value, "trivially copy constructible");
static_assert(std::is_trivially_move_constructible<Plain>::value, "trivially move constructible");
static_assert(std::is_trivially_copy_assignable<int>::value, "int is trivially copy assignable");
static_assert(std::is_trivially_copy_assignable<Plain>::value, "plain is trivially copy assignable");
static_assert(std::is_trivially_move_assignable<Plain>::value, "plain is trivially move assignable");
static_assert(!std::is_trivially_copy_assignable<const int>::value, "const is not assignable");
static_assert(std::is_trivially_destructible_v<Plain>, "is_trivially_destructible_v");
static_assert(std::is_trivially_copy_assignable_v<Plain>, "is_trivially_copy_assignable_v");

int main()
{
  return 0;
}
