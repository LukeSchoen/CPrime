struct Vault {
private:
    template<class T> static int read(T value) { return value + 1; }
public:
    using Reader = int (*)(int);
    static Reader pointer() { return read; }
    static int own() { return read(3); }
    static int extent() { return sizeof(read(0)); }
    friend int trusted();
protected:
    template<class T> static int inherited(T value) { return value + 2; }
};
int trusted() { return Vault::read(5); }
struct Derived : Vault {
    static int use() { return inherited(7); }
};
struct Hidden : Vault {
    static int inherited(int value) { return value + 3; }
    static int use() { return inherited(7); }
};
int main() {
    return Vault::own() != 4 || Vault::extent() != sizeof(int)
        || trusted() != 6 || Derived::use() != 9 || Hidden::use() != 10
        || Vault::pointer()(10) != 11;
}
