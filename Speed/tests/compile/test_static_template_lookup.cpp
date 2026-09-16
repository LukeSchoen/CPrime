// PROFILE_NAME: cpp.template.static.lookup
// Populate unrelated template families, then repeatedly resolve a static call.
// Changing family count exercises lookup scaling without external headers.
#define FAMILY(N) template<class T> struct Family##N { static int value(); }; \
    template<class T> int Family##N<T>::value() { return sizeof(T); } \
    Family##N<char> object##N;
#define GROUP(M, N) M(N##0) M(N##1) M(N##2) M(N##3) M(N##4) M(N##5) M(N##6) M(N##7)
#define GROUP_INNER(M, N) M(N##0) M(N##1) M(N##2) M(N##3) M(N##4) M(N##5) M(N##6) M(N##7)
#define GROUP_LEAF(M, N) M(N##0) M(N##1) M(N##2) M(N##3) M(N##4) M(N##5) M(N##6) M(N##7)
#define FAMILIES(N) GROUP_INNER(FAMILY, N)
GROUP(FAMILIES, 1)
GROUP(FAMILIES, 2)
GROUP(FAMILIES, 3)
GROUP(FAMILIES, 4)
template<class T> struct Target { static int value(); };
template<class T> int Target<T>::value() { return sizeof(T); }
#define CALL(N) result += Target<int>::value();
#define CALLS(N) GROUP_LEAF(CALL, N)
#define MANY_CALLS(N) GROUP_INNER(CALLS, N)
int main() {
    int result = 0;
    GROUP(MANY_CALLS, 1)
    return result != 2048;
}
