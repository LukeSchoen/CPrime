// EXPECT_COMPILE_FAIL: 1
template<class T> struct Sink {
    void accept(T &&value) {}
};
int main() { Sink<int> sink; int value = 4; sink.accept(value); }
