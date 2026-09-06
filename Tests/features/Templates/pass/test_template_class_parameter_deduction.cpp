template<class T> struct Box {
  T value;
  Box(T initial) : value(initial) {}
};
struct Derived : Box<int> { Derived() : Box<int>(17) {} };
template<class T> int read(const Box<T> &value) { return value.value; }
template<class T> int choose(const Box<T> &) { return 1; }
int choose(int) { return 2; }
int main()
{
  Derived value;
  if (read(value) != 17) return 1;
  if (read<int>(23) != 23) return 2;
  if (choose(11) != 2 || choose(value) != 1) return 3;
  return 0;
}
