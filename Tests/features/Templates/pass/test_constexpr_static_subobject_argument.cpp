struct Inner { int member; };
struct Outer { int prefix; Inner inner; };
constexpr Outer source{2, {4}};
constexpr int change(Inner value) { value.member += 3; return value.member; }
static_assert(change(source.inner) == 7);
int main() { return change(source.inner) != 7 || source.inner.member != 4; }
