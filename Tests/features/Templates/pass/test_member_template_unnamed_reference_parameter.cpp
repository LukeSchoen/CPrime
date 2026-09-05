template<class T> struct Holder {
  template<class B> bool has_bounds(B&) const { return false; }
};
struct Bounds { double low, high; };
int main() {
  const Holder<int> holder;
  Bounds bounds = { 3, 7 };
  return holder.has_bounds(bounds) || bounds.low != 3 || bounds.high != 7;
}
