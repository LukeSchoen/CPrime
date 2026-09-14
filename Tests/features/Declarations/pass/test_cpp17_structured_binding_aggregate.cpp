// EXPECT_COMPILE_ARGS: -std=c++17
template<class A, class B> struct same_type { static constexpr bool value = false; };
template<class A> struct same_type<A, A> { static constexpr bool value = true; };
template<class A, class B> constexpr bool same = same_type<A, B>::value;
struct Record { int number; int &reference; unsigned bits : 3; };
struct Derived : Record {};
int main() {
  int referred = 5;
  Record record = {3, referred, 6};
  auto [number, reference, bits] = record;
  number = 9;
  reference = 7;
  bits = 2;
  if (record.number != 3 || referred != 7 || record.bits != 6) return 1;
  static_assert(same<decltype(reference), int &>);
  const auto &[cn, cr, cb] = record;
  static_assert(same<decltype(cn), const int>);
  static_assert(same<decltype(cr), int &>);
  static_assert(same<decltype(cb), const unsigned>);
  decltype(bits) full_width = 257;
  if (full_width != 257) return 3;
  cr = 8;
  if (&cn != &record.number || referred != 8 || cb != 6) return 2;
  Derived derived = {{11, referred, 4}};
  auto &[dn, dr, db] = derived;
  dn = 12;
  dr = 10;
  db = 5;
  return derived.number != 12 || referred != 10 || derived.bits != 5;
}
