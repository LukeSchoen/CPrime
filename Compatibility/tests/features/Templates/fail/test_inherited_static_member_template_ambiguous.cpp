// EXPECT_COMPILE_FAIL: 1
struct Left { template<class T> static int read(T) { return 1; } };
struct Right { template<class T> static int read(T) { return 2; } };
struct Combined : Left, Right {
    static int use() { return read(0); }
};
int main() { return Combined::use(); }
