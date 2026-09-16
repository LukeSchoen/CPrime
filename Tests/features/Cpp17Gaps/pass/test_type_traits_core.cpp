// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: type_traits_core. <type_traits> carries the traits the runtime
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

int main()
{
  return 0;
}
