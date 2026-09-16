// EXPECT_COMPILE_ARGS: -std=c++17
// <functional> keeps the deprecated function base classes that C++17 still
// requires, with their argument and result typedefs; Boost.ContainerHash's
// hash_base derives from std::unary_function<T, std::size_t> and the compiler
// reported "base class type expected" while parsing that base clause.
#include <cstddef>
#include <functional>

template<class T>
struct hash_base : std::unary_function<T, std::size_t> { };

template<class T>
struct compare_base : std::binary_function<T, T, bool> { };

int main()
{
	hash_base<int>::argument_type value = 3;
	compare_base<int>::first_argument_type left = 1;
	compare_base<int>::second_argument_type right = 2;
	return value == left + right ? 0 : 1;
}
