/* A qualified id that reaches a static data member through a member typedef
   of a class-template instantiation (`B<T>::C::p`) continues at the class the
   typedef names.  The typedef of an instantiation is published under its
   scoped alias token rather than the joined static-member spelling the
   qualified path starts from, so the alias stood in for the whole name: a
   comparison that opened a statement or a `?:` condition compared the class
   value and reported `invalid operand types for binary operation`, while the
   same comparison behind a `!` or inside parentheses worked. */

template <const char *N> struct A { static const char *p; };
template <const char *N> const char *A<N>::p = N;

template <class T> struct B { static const char c[2]; typedef A<B<T>::c> C; };
template <class T> const char B<T>::c[2] = "b";

template <class T> struct D { static const char c[2]; typedef A<c> C; };
template <class T> const char D<T>::c[2] = "d";

int main() {
  /* The shapes that already worked keep reaching the array's address. */
  if (!(B<int>::C::p == B<int>::c)) return 1;
  if (!(D<int>::C::p == D<int>::c)) return 2;
  if (B<int>::C::p[0] != 'b') return 3;
  if (D<int>::C::p[0] != 'd') return 4;

  /* The comparison opens a `?:` condition, for a true and a false result. */
  if ((B<int>::C::p == B<int>::c ? 1 : 0) != 1) return 5;
  if ((B<float>::C::p == B<int>::c ? 1 : 0) != 0) return 6;

  /* The comparison opens an initializer. */
  bool same = B<int>::C::p == B<int>::c;
  if (!same) return 7;

  /* The comparison opens an `if` condition, and a discarded statement. */
  B<int>::C::p == B<int>::c;
  if (B<int>::C::p != B<int>::c) return 8;

  /* Distinct instantiations still name distinct arrays. */
  if ((D<float>::C::p != D<int>::c ? 1 : 0) != 1) return 9;
  if (B<float>::C::p[0] != 'b') return 10;

  /* The lead's spelling: the comparison is the whole return operand. */
  return B<int>::C::p == B<int>::c ? 0 : 11;
}
