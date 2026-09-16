// EXPECT_COMPILE_ARGS: -Werror
template<class T> struct Incomplete;
template<class T> struct Factory {
    static int make(int value);
    static int make(const Incomplete<T> &value);
    static int other(int value);
};
template<class T> int Factory<T>::make(int value) { return value + 1; }
template<class T> int Factory<T>::make(const Incomplete<T> &value) { return value.member; }
template<class T> int Factory<T>::other(int value) { return value + 2; }
int main() {
    if (Factory<double>::make(6) != 7) return 1;
    int (*selected)(int) = &Factory<double>::other;
    return selected(6) != 8;
}
