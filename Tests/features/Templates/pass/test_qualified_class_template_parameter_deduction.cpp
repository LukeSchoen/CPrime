namespace shapes { namespace detail {
template<class T> struct Value { T number; };
} }

template<class T> T read(const shapes::detail::Value<T>& value) {
  return value.number;
}
namespace shapes {
template<class T> T readRelative(const detail::Value<T>& value) {
  return value.number;
}
}

int main() {
  shapes::detail::Value<int> value = {17};
  if (read(value) != 17) return 1;
  if (shapes::readRelative(value) != 17) return 2;
  return 0;
}
