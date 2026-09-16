constexpr int through_pointer() { int value = 7; int *p = &value; int &r = value; return *p + r; }
constexpr bool identity() { int first = 1; int second = 2; const int *p = &first; return p == &first && p != &second; }
static_assert(through_pointer() == 14);
static_assert(identity());
int main() { return through_pointer() != 14 || !identity(); }