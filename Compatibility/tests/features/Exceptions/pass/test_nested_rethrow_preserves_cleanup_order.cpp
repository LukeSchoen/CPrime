// EXPECT_EXIT: 0
int order;
struct Guard { int id; Guard(int n) : id(n) {} ~Guard() { order = order * 10 + id; } };
int main() {
    try {
        Guard outer(1);
        try { Guard inner(2); throw 9; }
        catch (int value) { if (value != 9 || order != 2) return 1; throw; }
    } catch (int value) { if (value != 9 || order != 21) return 2; }
    return order != 21;
}
