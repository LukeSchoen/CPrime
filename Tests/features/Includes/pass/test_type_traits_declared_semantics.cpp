#include <type_traits>
static_assert(!std::is_same<const int, int>::value);
static_assert(std::is_signed<double>::value);
static_assert(std::is_unsigned<bool>::value);
static_assert(!std::is_signed<bool>::value);
namespace user {
  template<class T> struct is_integral { static constexpr bool value = true; };
}
template<bool V> struct Result { static constexpr bool value = V; };
template<class T> constexpr bool declared_value = Result<user::is_integral<T>::value>::value;
int main() { static_assert(declared_value<double>); }
