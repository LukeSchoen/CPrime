// EXPECT_EXIT: 0
// An unnamed struct/union that declares member functions has no class name
// token, so every member declaration used to be rejected with `member
// functions require a named struct/class`; and the lowered `this` parameter of
// an in-class union member body was spelled `struct U *`, which made the
// re-parse of that parameter report `redeclaration of 'U'`.  Both forms must
// declare, define and call their members.
static struct {
    int value;
    void set(int v) { value = v; }
    int get() const { return value; }
} anonymous_object;

union named_union {
    int i;
    void set(int v) { i = v; }
    int get() const { return i; }
};

int main() {
    anonymous_object.set(7);
    if (anonymous_object.get() != 7)
        return 1;
    named_union u;
    u.set(9);
    if (u.get() != 9)
        return 2;
    return 0;
}
