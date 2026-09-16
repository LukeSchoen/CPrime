struct First { virtual int tag() { return 1; } };
struct Second {
    int value;
    Second() : value(42) {}
    bool operator!() { return value == 42; }
    int operator-() { return -value; }
    operator void*() { return this; }
};
struct Derived : First, Second {};
int main() {
    Derived object;
    if (!(void*)object || (void*)object != (Second*)&object) return 1;
    return !(!object) || -object != -42;
}
