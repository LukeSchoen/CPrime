// EXPECT_EXIT: 0
typedef int (*Pointer)(int);
typedef int (&Reference)(int);
int add(int value) { return value + 1; }
int subtract(int value) { return value - 1; }
struct PointerCall {
    operator Pointer() { return add; }
    operator Pointer() const { return subtract; }
};
struct ReferenceCall { operator Reference() { return add; } };
struct StoredCall {
    Pointer pointer;
    StoredCall() : pointer(add) {}
    operator const Pointer&() const { return pointer; }
};
int main() {
    PointerCall plain;
    const PointerCall constant;
    ReferenceCall reference;
    StoredCall stored;
    return plain(3) != 4 || constant(3) != 2
        || reference(3) != 4 || stored(3) != 4;
}
