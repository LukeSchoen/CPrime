// EXPECT_EXIT: 0
enum Small : unsigned short { maximum = 65535 };
enum Signed { negative = -1, positive = 1 };
enum class Wide : unsigned long long { high = 0x100000000ULL };
enum class Opaque;
template<class E> struct Underlying { typedef __underlying_type(E) type; };
template<class A, class B> struct Same { enum { value = 0 }; };
template<class A> struct Same<A, A> { enum { value = 1 }; };
__underlying_type(Small) small = maximum;
Underlying<Wide>::type wide = 0x100000000ULL;
static_assert(Same<__underlying_type(const Small), unsigned short>::value, "fixed type");
static_assert(Same<__underlying_type(Signed), int>::value, "signed type");
static_assert(Same<__underlying_type(Opaque), int>::value, "opaque scoped enum");
int main() {
  return small != 65535 || wide != 0x100000000ULL
    || __underlying_type(Signed)(negative) != -1;
}
