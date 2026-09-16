/* Standard, header-free C++: 512 instantiated class bodies and out-of-line
   functions. Keep definitions live through observable, checked calls. */
#define JOIN_I(a,b) a##b
#define JOIN(a,b) JOIN_I(a,b)
#define ROW(M,R) M(JOIN(R,a)) M(JOIN(R,b)) M(JOIN(R,c)) M(JOIN(R,d)) \
                 M(JOIN(R,e)) M(JOIN(R,f)) M(JOIN(R,g)) M(JOIN(R,h))
#define INNER(M,R) M(JOIN(R,a)) M(JOIN(R,b)) M(JOIN(R,c)) M(JOIN(R,d)) \
                   M(JOIN(R,e)) M(JOIN(R,f)) M(JOIN(R,g)) M(JOIN(R,h))
#define ROWS(M) M(a) M(b) M(c) M(d) M(e) M(f) M(g) M(h)
template<class Tag> struct Value {
    unsigned data;
    explicit Value(unsigned x) : data(x) {}
    unsigned mix(unsigned x) const { return (x * 33u + data) ^ (x >> 3); }
};
#define DECLARE(N) struct JOIN(Tag_,N) {}; unsigned JOIN(fn_,N)(unsigned x) { \
    Value<JOIN(Tag_,N)> v(x + 7u); \
    for (unsigned i=0; i<16; ++i) x = v.mix(x + i); return x; }
#define DECLARE_ROW(N) INNER(DECLARE,N)
#define DECLARE_BLOCK(N) ROW(DECLARE_ROW,N)
ROWS(DECLARE_BLOCK)
static unsigned reference(unsigned x) {
    unsigned data=x+7u;
    for (unsigned i=0; i<16; ++i) { unsigned y=x+i; x=(y*33u+data)^(y>>3); }
    return x;
}
#define CHECK(N) if (JOIN(fn_,N)(seed) != expected) return 1;
#define CHECK_ROW(N) INNER(CHECK,N)
#define CHECK_BLOCK(N) ROW(CHECK_ROW,N)
int main(int argc, char **) {
    unsigned seed=(unsigned)argc, expected=reference(seed);
    ROWS(CHECK_BLOCK)
    return 0;
}
