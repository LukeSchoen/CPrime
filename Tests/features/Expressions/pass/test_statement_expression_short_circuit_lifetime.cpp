// EXPECT_EXIT: 0
int live, destroyed, calls;
struct Guard { Guard() { ++live; } ~Guard() { --live; ++destroyed; } };
int work() { ++calls; return 7; }
void visit(bool enabled) { enabled && (Guard(), ({ static int value = work(); value; })); }
int main() {
    visit(false);
    if (live || destroyed || calls) return 1;
    visit(true); visit(true);
    return live != 0 || destroyed != 2 || calls != 1;
}
