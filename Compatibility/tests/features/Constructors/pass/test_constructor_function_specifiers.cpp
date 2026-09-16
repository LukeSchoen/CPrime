struct Point {
  int value;
  constexpr Point() : value() {}
  explicit constexpr Point(int value) : value(value) {}
  constexpr static int constant = 3;
  constexpr int get() const { return value; }
  inline Point copy() const { return Point(value); }
};
struct Outside {
  int value;
  constexpr Outside(int value);
};
constexpr Outside::Outside(int value) : value(value) {}
template<class T> struct Box {
  T value;
  constexpr Box() : value() {}
  constexpr explicit Box(T value) : value(value) {}
};
int main() {
  Point zero;
  Point value(7);
  Outside outside(5);
  Box<int> empty;
  Box<int> box(9);
  return zero.value || value.get() != 7 || value.copy().value != 7
      || outside.value != 5 || empty.value || box.value != 9
      || Point::constant != 3;
}
