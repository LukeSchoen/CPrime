// A qualified self-type in a class-template member must retain the template
// name during instantiation instead of replaying it as a tagged class type.
namespace test_ns
{
template <typename T>
struct self
{
  static ::test_ns::self<T> const value;
};
}

test_ns::self<int> object;

int main()
{
  return 0;
}
