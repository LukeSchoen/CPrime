// EXPECT_EXIT: 0
struct Holder { int (*call)(); };
Holder holder = { []() { return 4; } };
int main() { return holder.call() != 4; }
