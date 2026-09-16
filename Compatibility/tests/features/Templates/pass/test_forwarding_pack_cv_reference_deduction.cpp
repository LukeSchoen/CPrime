// EXPECT_COMPILE_ARGS: -Werror
#include <new>
#include <type_traits>

struct Value {
  int selected;
  explicit Value(int): selected(0) {}
  Value(const Value&): selected(1) {}
  Value(Value&): selected(2) {}
  Value(Value&&): selected(3) {}
};
template<class T, class... Args>
void construct(T* destination, Args&&... args) {
  new(destination) T(static_cast<Args&&>(args)...);
}
template<class T> void copy(T* destination, const T* source) {
  construct(destination, source[0]);
}
template<class T> struct Category { static const int value = 0; };
template<class T> struct Category<T&> { static const int value = 1; };
template<class T> struct Category<const T&> { static const int value = 3; };
template<class T> int category(T&&) { return Category<T>::value; }
template<class... T> int pointer_pack(const T*... values) {
  return sizeof...(T);
}
int main() {
  Value source(0), destination(0);
  copy(&destination, &source);
  if (destination.selected != 1) return 1;
  construct(&destination, source);
  if (destination.selected != 2) return 2;
  construct(&destination, Value(0));
  if (destination.selected != 3) return 3;
  const int fixed = 4;
  int mutable_value = 5;
  if (category(fixed) != 3 || category(mutable_value) != 1
      || category(6) != 0 || category('x') != 0)
    return 4;
  typedef int& L;
  typedef int&& R;
  if (!std::is_same<L&&, int&>::value || !std::is_same<R&, int&>::value
      || !std::is_same<R&&, int&&>::value) return 5;
  return pointer_pack(&fixed, &mutable_value) != 2;
}
