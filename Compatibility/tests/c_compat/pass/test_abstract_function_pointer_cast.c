typedef unsigned long long U;
void f(void *p) {
    U (*fn)(void);
    fn = (U (*)(void))p;
}
static U answer(void) { return 42; }
int main(void) {
    void (*p)(void) = (void (*)(void))answer;
    U (*fn)(void) = (U (*)(void))p;
    if (fn() != 42) return 1;
    if (sizeof(U (*)(void)) != sizeof(fn)) return 2;
    return 0;
}
