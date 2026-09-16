typedef unsigned long long U;
static U answer() { return 42; }
static int increment(int x) { return x + 1; }
struct Value {
    int n;
    Value(int x) : n(x) {}
};
int main() {
    void (*p)() = (void (*)())answer;
    U (*fn)() = (U (*)())p;
    if (fn() != 42) return 1;
    if (((int (*)(int))increment)(6) != 7) return 2;
    U value = 3;
    if ((U(value) + 2) != 5) return 3;
    if ((Value(7)).n != 7) return 4;
    if (sizeof(U (*)(void)) != sizeof(fn)) return 5;
    return 0;
}
