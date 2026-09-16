// EXPECT_EXIT: 0
// `T (x)(init);` is a parenthesized declarator with a direct initializer;
// a third group continues an expression call chain.
struct F
{
    F(int) { }
    F(int, int) { }
    F operator()(int) const { return *this; }
    F operator()(int, int) const { return *this; }
};

int main()
{
    int i = 0;
    int (j)(1);
    F(i)(1)(2);
    F(i)(1, 2)(3);
    F(i)(1)(2, 3);
    return j != 1;
}
