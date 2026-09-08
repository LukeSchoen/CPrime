// EXPECT_COMPILE_FAIL: 1
struct Vault {
private:
    template<class T> static int read(T);
};
int probe() { return sizeof(Vault::read(3)); }
