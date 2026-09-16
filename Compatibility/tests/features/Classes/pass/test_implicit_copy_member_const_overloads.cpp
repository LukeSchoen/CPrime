static int selected;
struct Member {
    Member() {}
    Member(Member&) { selected = 1; }
    Member(const Member&) { selected = 2; }
    Member& operator=(Member&) { selected = 3; return *this; }
    Member& operator=(const Member&) { selected = 4; return *this; }
};
struct Derived : Member {};
struct Container { Member member; };
struct MutableMemberContainer { mutable Member member; };
struct MutableOnly {
    MutableOnly() {}
    MutableOnly(MutableOnly&) { selected = 5; }
    MutableOnly& operator=(MutableOnly&) { selected = 6; return *this; }
};
struct MutableContainer { MutableOnly member; };
int main() {
    Derived source;
    Derived copy(source);
    if (selected != 2) return 1;
    Container first;
    Container second(first);
    if (selected != 2) return 2;
    first = second;
    if (selected != 4) return 3;
    const Container constant;
    Container third(constant);
    if (selected != 2) return 4;
    third = constant;
    if (selected != 4) return 5;
    MutableContainer mutable_source;
    MutableContainer mutable_copy(mutable_source);
    if (selected != 5) return 6;
    mutable_source = mutable_copy;
    if (selected != 6) return 7;
    const MutableMemberContainer mutable_member_source;
    MutableMemberContainer mutable_member_copy(mutable_member_source);
    if (selected != 1) return 8;
    mutable_member_copy = mutable_member_source;
    return selected != 3;
}
