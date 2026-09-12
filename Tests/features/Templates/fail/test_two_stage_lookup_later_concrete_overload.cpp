// EXPECT_COMPILE_FAIL: 1
// Two-stage lookup: a dependent call whose name the template definition already
// bound must not reach an overload declared later in the same namespace, and a
// built-in argument type associates no namespaces for ADL to search.
//
// This is the standalone reproduction of the sibling CL build failure in
// FreeLancer (CommonLib/src/Polygon/clMesh.cpp): clVector2.h defined
// clHash(const clVector2<T> &) before clHash.h declared the scalar clHash
// overloads, so the unqualified call inside that body had no viable candidate
// in any translation unit that included clVector2.h first.  MSVC's permissive
// mode accepted it by resolving at instantiation time; the definition-visible
// candidate set decides it here, so the call must be rejected.

template<typename T> struct Vec2 { T x; };

template<typename T> long clHash(const Vec2<T> &value);
template<typename T> long clHash(const Vec2<T> &value) { return clHash(value.x); }

long clHash(const long &value);

long use() { Vec2<double> value{}; return clHash(value); }
