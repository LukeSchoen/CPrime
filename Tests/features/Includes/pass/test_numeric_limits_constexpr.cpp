#include <limits>
#include <type_traits>
template<class T> constexpr auto maximum() { return std::numeric_limits<typename std::remove_reference<T>::type>::max(); }
struct Task { static const long long never = maximum<long long>(); };
static_assert(Task::never == 9223372036854775807LL, "64 bit constant");
static_assert(std::numeric_limits<unsigned int>::max() == 4294967295U, "unsigned");
static_assert(std::numeric_limits<int>::lowest() == (-2147483647 - 1), "signed");
int main() { return std::numeric_limits<double>::min()>0 && std::numeric_limits<double>::max()>1e300 ? 0 : 1; }
