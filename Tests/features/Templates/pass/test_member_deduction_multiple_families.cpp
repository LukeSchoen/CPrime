template<int N> struct Tag {};
struct Selector {
#define FAMILY(N) template<class T> int pick(const T&, Tag<N>) { return N; }
  FAMILY(0) FAMILY(1) FAMILY(2) FAMILY(3) FAMILY(4)
  FAMILY(5) FAMILY(6) FAMILY(7) FAMILY(8) FAMILY(9)
  FAMILY(10) FAMILY(11) FAMILY(12) FAMILY(13) FAMILY(14)
  FAMILY(15) FAMILY(16) FAMILY(17) FAMILY(18) FAMILY(19)
#undef FAMILY
};
int main() {
  Selector selector;
#define CHECK(N) if (selector.pick(1, Tag<N>()) != N) return 1;
  CHECK(0) CHECK(1) CHECK(2) CHECK(3) CHECK(4)
  CHECK(5) CHECK(6) CHECK(7) CHECK(8) CHECK(9)
  CHECK(10) CHECK(11) CHECK(12) CHECK(13) CHECK(14)
  CHECK(15) CHECK(16) CHECK(17) CHECK(18) CHECK(19)
#undef CHECK
  return selector.pick(2.5, Tag<0>()) != 0
      || selector.pick(1, Tag<19>()) != 19;
}
