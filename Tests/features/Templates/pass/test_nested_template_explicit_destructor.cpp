// EXPECT_EXIT: 0
#include <new>
int destroyed;
template<class T> struct Outer {
  template<class U> struct Inner { ~Inner() { ++destroyed; } };
};
template<class T, class U> void destroy(typename Outer<T>::template Inner<U>* value) {
  value->Outer<T>::template Inner<U>::~Inner();
}
int main() {
  Outer<int>::Inner<char>* value = new Outer<int>::Inner<char>;
  destroy<int, char>(value);
  ::operator delete(value);
  return destroyed != 1;
}
