#include <memory>
#include <type_traits>

int main()
{
  typedef std::allocator<int> allocator_type;
  static_assert(std::is_same<allocator_type::value_type, int>::value,
                "allocator value_type");
  static_assert(std::is_same<allocator_type::pointer, int*>::value,
                "allocator pointer");
  static_assert(std::is_same<allocator_type::const_pointer, const int*>::value,
                "allocator const_pointer");
  static_assert(std::is_same<allocator_type::reference, int&>::value,
                "allocator reference");
  static_assert(std::is_same<allocator_type::const_reference, const int&>::value,
                "allocator const_reference");
  static_assert(std::is_same<allocator_type::size_type, std::size_t>::value,
                "allocator size_type");
  static_assert(std::is_same<allocator_type::difference_type, std::ptrdiff_t>::value,
                "allocator difference_type");
  return 0;
}
