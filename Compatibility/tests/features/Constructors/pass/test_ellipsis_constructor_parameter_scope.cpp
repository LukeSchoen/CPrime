// EXPECT_EXIT: 0
int calls;
struct Record { Record(...) { ++calls; } };
int main() { Record a(1, 2), b('x'); return calls != 2; }
