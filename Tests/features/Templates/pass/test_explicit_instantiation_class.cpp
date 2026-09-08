// EXPECT_EXIT: 0
template<class T> struct Box {
    T value;
    T get() const { return value; }
    void declared_only();
};
template struct Box<int>;
template<class T> T twice(T x) { return x + x; }
template int twice(int);
int main() { Box<int> b; b.value = 21; return twice(b.get()) != 42; }
