static int order;
struct Member {
    int value;
    Member(int value = 1) : value(value) {}
    ~Member() { order = order * 10 + value; }
};
struct Base {
    Member member;
    Base() : member(1) {}
    ~Base() { order = order * 10 + 2; }
};
struct Explicit : Base {
    Member members[2];
    Explicit() { members[0].value = 3; members[1].value = 4; }
    ~Explicit() { order = order * 10 + 5; }
};
struct Implicit {
    Member first;
    Member last;
    Implicit() : first(6), last(7) {}
};
int main() {
    { Explicit object; }
    if (order != 54321) return 1;
    order = 0;
    { Implicit object; }
    if (order != 76) return 2;
    order = 0;
    try { Implicit object; throw object; }
    catch (const Implicit& object) {
        if (order != 76 || object.first.value != 6 || object.last.value != 7) return 3;
    }
    return order != 7676;
}
