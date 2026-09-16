// EXPECT_EXIT: 0
template<int (*F)(int)> struct Dispatch {
    static int run(int n) { return F(n); }
};
struct Owner {
    static int transform(int n) { return n + 3; }
    int transform() { return Dispatch<transform>::run(7); }
};
int main() { Owner owner; return owner.transform() != 10; }
