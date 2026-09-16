struct Inner { mutable int member; int fixed; };
struct Value { Inner inner; };
constexpr int run() { const Value value{{3, 4}}; value.inner.member = 7; return value.inner.member + value.inner.fixed; }
static_assert(run() == 11);
int main() { return run() != 11; }
