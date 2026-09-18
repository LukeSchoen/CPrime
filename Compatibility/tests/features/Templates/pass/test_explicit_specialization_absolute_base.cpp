// An explicit class template specialization may name an absolute base-class
// type: the base-clause colon is followed by the global-qualified name.
namespace BaseNamespace
{
  struct Base
  {
    int value() const { return 17; }
  };
}

template <typename> struct Value;

template <>
struct Value<int> : ::BaseNamespace::Base
{
};

int main()
{
  Value<int> value;
  return value.value() == 17 ? 0 : 1;
}
