struct Value { int first = 4; int second = first + 3; };
static_assert(Value{}.second == 7);
struct Outer { int prefix; Value member; };
static_assert(Outer{}.member.second == 7);
constexpr int initialized = Outer{}.member.second;
static_assert(initialized == 7);
int main() {}
