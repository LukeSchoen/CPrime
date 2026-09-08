// EXPECT_EXIT: 0
int constructed, destroyed;
struct Item {
    Item() { if (++constructed == 37) throw 42; }
    ~Item() { ++destroyed; }
};
#define ITEMS(n) Item a##n; Item b##n; Item c##n; Item d##n;
struct Many {
    ITEMS(0) ITEMS(1) ITEMS(2) ITEMS(3)
    ITEMS(4) ITEMS(5) ITEMS(6) ITEMS(7)
    ITEMS(8) ITEMS(9) ITEMS(10) ITEMS(11)
    ITEMS(12) ITEMS(13) ITEMS(14) ITEMS(15)
};
int main() {
    try { Many value; return 1; }
    catch (int n) { if (n != 42) return 2; }
    if (constructed != 37) return 3;
    if (destroyed != 36) return 4;
    return 0;
}
