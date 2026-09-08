// EXPECT_EXIT: 0
int calls;
struct Value { operator int() { ++calls; return 42; } };
int choose(bool b, Value &v, const int &n) { return b ? v : n; }
int reverse(bool b, Value &v, const int &n) { return b ? n : v; }
int main() {
    Value v;
    const int n = 7;
    if (choose(true, v, n) != 42 || calls != 1) return 1;
    if (choose(false, v, n) != 7 || calls != 1) return 2;
    if (reverse(false, v, n) != 42 || calls != 2) return 3;
    if (reverse(true, v, n) != 7 || calls != 2) return 4;
    return 0;
}
