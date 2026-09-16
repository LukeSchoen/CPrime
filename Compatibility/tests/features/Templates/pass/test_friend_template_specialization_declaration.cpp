// EXPECT_EXIT: 0
int value;
template<class T> void record(T n) { value = n; }
template<> void record<char>(char n) { value = n + 1; }
template<class T> struct Observer {
    friend void record<T>(T);
    friend void record<char>(char);
    void run(T n) { record<T>(n); }
};
int main() {
    Observer<int> a;
    a.run(12);
    if (value != 12) return 1;
    Observer<char> b;
    b.run(20);
    return value != 21;
}
