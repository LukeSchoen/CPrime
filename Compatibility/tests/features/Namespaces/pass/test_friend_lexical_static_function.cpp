// EXPECT_EXIT: 0
struct Other { static int value() { return 1; } };
struct Owner {
    static int value() { return 17; }
    friend int inspect(Owner) { return value(); }
};
template<class T> struct Generic {
    static int value() { return 23; }
    friend int inspect(Generic) { return value(); }
};
int main() { return inspect(Owner()) != 17 || inspect(Generic<int>()) != 23; }
