// EXPECT_EXIT: 0
int observed;
struct Base;
struct Member { Base* owner; ~Member(); };
struct Base {
    Member member;
    Base() { member.owner = this; }
    virtual int value() { return 1; }
};
Member::~Member() { observed = observed * 10 + owner->value(); }
struct Prefix { virtual int other() { return 0; } };
struct Derived : Prefix, Base {
    int value() { return 2; }
    ~Derived() { observed = observed * 10 + value(); }
};
int main() {
    { Derived object; }
    if (observed != 21) return 1;
    observed = 0;
    try { Derived object; throw 7; } catch (int) {}
    return observed != 21;
}
