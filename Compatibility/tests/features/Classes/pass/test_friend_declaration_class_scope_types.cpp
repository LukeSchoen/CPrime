// EXPECT_EXIT: 0
struct Owner {
    typedef int Number;
    enum Kind { first = 13 };
    struct Value { int number; };
    friend int read(Number);
    friend int read(Kind);
    friend int read(Value);
};
int read(int n) { return n; }
int read(Owner::Kind n) { return n; }
int read(Owner::Value n) { return n.number; }
int main() { Owner::Value v = {17}; return read(v) + read(Owner::first) + read(3) != 33; }
