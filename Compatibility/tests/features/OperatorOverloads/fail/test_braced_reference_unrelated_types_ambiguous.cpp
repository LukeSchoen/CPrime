// EXPECT_COMPILE_FAIL: 1
struct First { int x, y; };
struct Second { int x, y; };
struct Sink {
    void add(const First &) {}
    void add(Second &&) {}
};
int main() { Sink sink; sink.add({1, 2}); }
