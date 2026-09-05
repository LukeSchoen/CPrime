static int construction, destruction, complete_destructor;
struct Member {
    int value;
    Member(int value) : value(value) {
        construction = construction * 10 + value;
        if (value == 2) throw 197;
    }
    ~Member() { destruction = destruction * 10 + value; }
};
struct Partial {
    Member first;
    Member second;
    Partial() : second(2), first(1) {}
    ~Partial() { ++complete_destructor; }
};
struct Base {
    Member member;
    Base() : member(4) {}
};
struct FromBody : Base {
    Member first;
    Member second;
    FromBody() : second(3), first(1) { throw 199; }
    ~FromBody() { ++complete_destructor; }
};
int main() {
    try { Partial object; }
    catch (int value) {
        if (value != 197 || construction != 12 || destruction != 1 || complete_destructor) return 1;
    }
    construction = destruction = 0;
    try { FromBody object; }
    catch (int value) {
        if (value != 199 || construction != 413 || destruction != 314 || complete_destructor) return 2;
    }
    return 0;
}
