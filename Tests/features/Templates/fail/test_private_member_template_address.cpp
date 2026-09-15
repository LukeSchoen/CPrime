// EXPECT_COMPILE_FAIL: 1
struct Vault {
private:
    template<class T> static int read(T value) { return value; }
};
int (*reader)(int) = &Vault::read;
int main() { return reader(0); }
