// EXPECT_EXIT: 0
int live, built, destroyed, fail_at;
struct Element {
    Element(int value) { if (value == fail_at) throw value; ++live; ++built; }
    ~Element() { --live; ++destroyed; }
};
struct Group { Element pair[2]; Element tail; };
int main() {
    for (int target = 1; target <= 6; ++target) {
        fail_at = target; live = built = destroyed = 0;
        try { Group groups[2] = { { {1, 2}, 3 }, { {4, 5}, 6 } }; return 1; }
        catch (int value) { if (value != target) return 2; }
        if (live || built != target - 1 || destroyed != built) return 3;
    }
    return 0;
}
