template<class Owner> struct Counter {
  template<class... Args> int count(Args... args) { return sizeof...(Args); }
};
int first(Counter<int> &c) { return c.count(1, 2, 3); }
int main() {
  Counter<int> c;
  Counter<short> other;
  if (first(c) != 3 || c.count() != 0) return 1;
  if (c.count(1,2,3,4,5,6,7,8,9,10,11,12) != 12) return 2;
  if (c.count(1) != 1 || c.count(1,2) != 2) return 3;
  if (c.count(1,2,3,4) != 4 || c.count(1,2,3,4,5) != 5) return 4;
  if (c.count(1,2,3,4,5,6) != 6 || c.count(1,2,3,4,5,6,7) != 7) return 5;
  if (c.count(1,2,3,4,5,6,7,8) != 8) return 6;
  return other.count(1.0, 'a') != 2 || first(c) != 3 || c.count() != 0;
}
