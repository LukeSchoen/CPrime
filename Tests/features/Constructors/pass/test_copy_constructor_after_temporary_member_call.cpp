// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <initializer_list>

template<class T> struct List {
  int count;
  List();
  List(const std::initializer_list<T>&);
  List(const List&);
  List(List&&);
  ~List();
};
template<class T> List<T>::List(): count(7) {}
template<class T> List<T>::List(const std::initializer_list<T>&): count(99) {}
template<class T> List<T>::List(const List& other): count(other.count) {}
template<class T> List<T>::List(List&& other): count(other.count) { other.count=0; }
template<class T> List<T>::~List() {}

struct Item { int value; };
struct Reader {
  List<Item> values;
  Reader(int, int) {}
  const List<Item>& Inputs() { return values; }
  const List<Item>& operator[](int) { return values; }
};
int selected(const List<Item>& value) { return value.count; }
int selected(const Reader&) { return 99; }

int main() {
  // The argument's type is the member call result, not the temporary receiver.
  List<Item> values = Reader(1, 2).Inputs();
  if (values.count != 7) return 1;
  if (selected(Reader(3, 4).Inputs()) != 7) return 2;
  if (selected(Reader(5, 6).values) != 7) return 3;
  if (selected(Reader(7, 8)[0]) != 7) return 4;
  return selected(Reader((9), (10))) != 99;
}
