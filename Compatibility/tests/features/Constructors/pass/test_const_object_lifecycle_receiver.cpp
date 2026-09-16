// EXPECT_COMPILE_ARGS: -Werror
int destroyed;
struct Value {
    const int number;
    Value() : number(3) {}
    Value(int n) : number(n) {}
    ~Value() { ++destroyed; }
};
struct Owner {
    const Value member;
    Owner() : member(11) {}
};
int main() {
    {
        const Value first(7);
        const Value second;
        const Value third = Value(9);
        const Owner fourth;
        if (first.number != 7 || second.number != 3
            || third.number != 9 || fourth.member.number != 11) return 1;
    }
    return destroyed != 4;
}
