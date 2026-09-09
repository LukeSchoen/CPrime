// Conversion lookup must not scan the unrelated classes' template members.
#define CAT_I(a, b) a##b
#define CAT(a, b) CAT_I(a, b)
#define ROW(M, R) \
  M(CAT(R,a)) M(CAT(R,b)) M(CAT(R,c)) M(CAT(R,d)) \
  M(CAT(R,e)) M(CAT(R,f)) M(CAT(R,g)) M(CAT(R,h)) \
  M(CAT(R,i)) M(CAT(R,j)) M(CAT(R,k)) M(CAT(R,l)) \
  M(CAT(R,m)) M(CAT(R,n)) M(CAT(R,o)) M(CAT(R,p))
#define ROWS(M) M(a) M(b) M(c) M(d) M(e) M(f) M(g) M(h) \
                M(i) M(j) M(k) M(l) M(m) M(n) M(o) M(p)
#define DECLARE(N) template<class T> struct CAT(Unrelated,N) { \
  template<class U> U member(U value) { return value; } };
#define DECLARE_ROW(R) ROW(DECLARE, R)
ROWS(DECLARE_ROW)
struct Number { template<class T> operator T() { return T(7); } };
#define USE(N) int CAT(use_,N)(Number value) { return value; }
#define USE_ROW(R) ROW(USE, R)
ROWS(USE_ROW)
int main() { Number number; return use_aa(number) != 7 || use_pp(number) != 7; }
