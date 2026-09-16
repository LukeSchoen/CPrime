// PROFILE_NAME: cpp.call.source.lookup
// Resolving an unqualified call must not walk the whole local stack looking
// for a synthetic member receiver, and must not scan every saved inline body
// for a callee of that name.  This case fills both tables and then resolves
// many calls from a function with a deep local stack.
#define CAT_I(a, b) a##b
#define CAT(a, b) CAT_I(a, b)
#define ROW(M, R) \
  M(CAT(R,a)) M(CAT(R,b)) M(CAT(R,c)) M(CAT(R,d)) \
  M(CAT(R,e)) M(CAT(R,f)) M(CAT(R,g)) M(CAT(R,h)) \
  M(CAT(R,i)) M(CAT(R,j)) M(CAT(R,k)) M(CAT(R,l)) \
  M(CAT(R,m)) M(CAT(R,n)) M(CAT(R,o)) M(CAT(R,p))
#define ROWS(M) M(a) M(b) M(c) M(d) M(e) M(f) M(g) M(h) \
                M(i) M(j) M(k) M(l) M(m) M(n) M(o) M(p)
#define ROW8(M, R) \
  M(CAT(R,a)) M(CAT(R,b)) M(CAT(R,c)) M(CAT(R,d)) \
  M(CAT(R,e)) M(CAT(R,f)) M(CAT(R,g)) M(CAT(R,h))
#define ROW8S(M) M(a) M(b) M(c) M(d) M(e) M(f) M(g) M(h)
#define BODY(N) static inline int CAT(inline_, N)(int value) { return value + 1; }
#define BODY_ROW(R) ROW8(BODY, R)
#define LOCAL(N) int CAT(local_, N) = 1;
#define LOCAL_ROW(R) ROW(LOCAL, R)
#define CALL(N) total = add(total) + CAT(local_, N);
#define CALL_ROW(R) ROW(CALL, R)
ROW8S(BODY_ROW)
static int add(int value) { return value + 1; }
int main(void)
{
  int total = 0;
  ROWS(LOCAL_ROW)
  ROWS(CALL_ROW)
  return total > 0 ? 0 : 1;
}
