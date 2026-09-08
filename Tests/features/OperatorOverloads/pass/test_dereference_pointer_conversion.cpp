// EXPECT_EXIT: 0
int item = 42;
struct Ptr {
    operator int*() const { return &item; }
    operator bool() const { return false; }
};
struct Overloaded : Ptr { int operator*() const { return 7; } };
int main() {
    Ptr p;
    if (*p != 42) return 1;
    *p = 19;
    if (item != 19) return 2;
    Overloaded o;
    return *o != 7;
}
