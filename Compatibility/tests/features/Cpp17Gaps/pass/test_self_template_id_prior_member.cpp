// EXPECT_COMPILE_ARGS: -std=c++17
// A self template-id whose argument names a static member declared earlier in
// the same class, which is what Boost.Integer's choose_initial_n does:
//
//   static const bool c = ...;
//   static const int value = !c * n + choose_initial_n<2 * c * n>::value;
//
// Only the declaration parser binds `c` in member order, so the replay cannot
// instantiate the id while it is still assembling the class text. It reported
// "'c' undeclared" (or "unsupported template type argument") instead.
typedef unsigned argument_type;
typedef int result_type;

template <result_type n>
struct choose_initial_n {
	static const bool c = (argument_type(1) << n << n) != 0;
	static const result_type value = !c * n + choose_initial_n<2 * c * n>::value;
};

template <>
struct choose_initial_n<0> {
	static const result_type value = 0;
};

int main()
{
	return choose_initial_n<16>::value == 16 ? 0 : 1;
}
