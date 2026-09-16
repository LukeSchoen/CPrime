// EXPECT_EXIT: 0
// A class-type member subobject needs its own dispatch table.  When the member
// class has a virtual base (or virtual functions) but only an implicit
// constructor, no constructor body is emitted for the member, so the enclosing
// object's construction path must publish that table.  Without it the member
// subobject's vptr stayed zero and the first virtual-base conversion
// (`h.member.i`, `Root* base = &h.member`) faulted at address 0.
struct Root {
    int i;
};

struct Intermediate : virtual Root {
    int j;
};

struct Holder {
    Intermediate member;
};

struct PolyRoot {
    int i;
};

struct PolyMember : virtual PolyRoot {
    int j;
    virtual int value() { return j; }
};

struct PolyHolder {
    PolyMember member;
};

int main() {
    Holder h;
    h.member.i = 42;
    if (h.member.i != 42)
        return 1;
    Intermediate* pointer = &h.member;
    Root* base = pointer;
    base->i = 7;
    if (h.member.i != 7)
        return 2;

    PolyHolder p;
    p.member.j = 5;
    if (p.member.value() != 5)
        return 3;
    return 0;
}
