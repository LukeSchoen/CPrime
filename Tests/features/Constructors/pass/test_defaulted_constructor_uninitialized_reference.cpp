struct Value { int &reference; Value() = default; };
static_assert(!__is_constructible(Value));
static_assert(!__is_trivially_constructible(Value));
struct Constant { const int value; Constant() = default; };
struct Nested { Value value; Nested() = default; };
int source;
struct Initialized { int &reference = source; Initialized() = default; };
static_assert(!__is_constructible(Constant));
static_assert(!__is_constructible(Nested));
static_assert(__is_constructible(Initialized));
int main() {}
