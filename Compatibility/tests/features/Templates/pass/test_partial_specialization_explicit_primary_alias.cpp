// An explicit template-id inside a partial specialization names the primary
// template, not the selected partial's injected-class-name type.
template <typename = int, typename = int, typename = int, typename = int,
          typename = int>
struct a;

template <typename> struct b;
template <typename = int, typename d = void> struct e : b<d>::c {
  typedef e f;
};
template <> struct b<void> { typedef e<> c; };
template <> struct e<> {};

template <typename i> struct a<i> : e<i> {};
template <typename i, typename j, typename k, typename l>
struct a<i, j, k, l> : e<typename a<j>::f> {
  static int value() { return 31; }
};

int main()
{
  return a<float, float>::value() == 31 ? 0 : 1;
}
