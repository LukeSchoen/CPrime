template<class T> struct Wrapper { T item; };
namespace lib {
  template<int N, class T> struct Select { using type = T; };
  template<class T> struct Select<-1, T> { using type = Wrapper<T>; };
}
struct Owner { struct Item { int x; }; };
int main() {
  lib::Select<-1, Owner::Item>::type value;
  value.item.x = 7;
  return value.item.x != 7;
}
