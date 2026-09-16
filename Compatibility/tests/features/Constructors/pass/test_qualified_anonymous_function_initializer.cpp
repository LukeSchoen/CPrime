namespace library { namespace detail { namespace {
int number() { return 17; }
}}}
struct Value { int n; explicit Value(int x):n(x){} };
int main() { Value v(library::detail::number()); return v.n != 17; }
