// EXPECT_EXIT: 0
int seed = 99;
struct Owner {
    enum { initial = 3 };
    static int seed;
    struct Nested { static int constant; static int dynamic; };
};
int Owner::seed = 7;
int Owner::Nested::constant = initial;
int Owner::Nested::dynamic = seed + 1;
int main() {
    return Owner::Nested::constant != 3 || Owner::Nested::dynamic != 8 || seed != 99;
}
