// EXPECT_EXIT: 0
int constructions;
struct Base { virtual ~Base() {} };
struct Member {
    int value;
    Member(int input) : value(input) { ++constructions; }
};
struct Derived : Base {
    Member member;
    Derived() : member(29) {}
};
int main() { Derived value; return value.member.value != 29 || constructions != 1; }
