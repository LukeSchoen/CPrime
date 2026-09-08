typedef char Yes;
struct No { char bytes[2]; };
template<class T> T produce();
template<bool, class T> struct Enable { typedef T type; };
template<class T> struct Enable<false, T> {};
template<class T>
typename Enable<(sizeof(produce<T>().read(), 1) > 0), Yes>::type detect(T const&);
No detect(...);
struct Reader { int read(); };
struct Writer { int read(int); };
static_assert(sizeof(detect(produce<Reader>())) == sizeof(Yes), "valid expression");
static_assert(sizeof(detect(produce<Writer>())) == sizeof(No), "invalid expression");
static_assert(sizeof(detect(produce<int Reader::*>())) == sizeof(No), "member pointer type-id");
template<class F, class A, class B>
typename Enable<sizeof(produce<F>()(produce<A>(), produce<B>()), 1), Yes>::type callable(int);
template<class, class, class> No callable(...);
struct Left;
struct Right { Right(Left); };
struct Left { Left(Right); };
struct Function { void operator()(Left, Left); void operator()(Right, Right); };
static_assert(sizeof(callable<Function, Left, Left>(0)) == sizeof(Yes), "exact overload");
static_assert(sizeof(callable<Function, Left, Right>(0)) == sizeof(No), "ambiguous overload");
static_assert(sizeof(callable<int(*)(int, int), int*, int>(0)) == sizeof(No), "pointer is not integer");
int main() { return 0; }
