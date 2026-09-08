// EXPECT_EXIT: 0
int live, created, destroyed, step;
struct Condition {
    int value;
    Condition(int v) : value(v) { ++live; ++created; }
    ~Condition() { --live; ++destroyed; }
    operator bool() const { return value != 0; }
};
int main() {
    for (int i = 0; Condition c = i < 3; ++i) {
        if (live != 1) return 1;
        ++step;
        if (i == 1) continue;
    }
    if (live || created != 4 || destroyed != 4 || step != 3) return 2;
    for (; Condition c = true; ) {
        if (live != 1) return 3;
        break;
    }
    return live || created != 5 || destroyed != 5;
}
