// EXPECT_COMPILE_FAIL: 1
// Two-stage lookup: a dependent call whose name the template definition already
// bound must not reach an overload declared later in the same namespace, and a
// built-in argument type associates no namespaces for ADL to search.
//
// The shape: a vector header defines `hashValue(const Vec2<T> &)` and calls the
// scalar `hashValue` from inside that body before the scalar overloads are
// declared, so the unqualified call has no viable candidate in any translation
// unit that includes the vector header first.  MSVC's permissive mode accepted
// it by resolving at instantiation time; the definition-visible candidate set
// decides it here, so the call must be rejected.

template<typename T> struct Vec2 { T x; };

template<typename T> long hashValue(const Vec2<T> &value);
template<typename T> long hashValue(const Vec2<T> &value) { return hashValue(value.x); }

long hashValue(const long &value);

long use() { Vec2<double> value{}; return hashValue(value); }
