// EXPECT_EXIT: 0
template<class T> struct Result { int kind() { return 1; } };
template<class T> struct Result<T &> {
    T *value;
    int kind() { return 2; }
};
template<class T> struct Result<T *> { int kind() { return 3; } };
template<class T> struct Result<T &&> { int kind() { return 4; } };
template<class T> struct State : Result<T> {};
template<class T, class U> struct Same { int kind() { return 0; } };
template<class T> struct Same<T *, T &> { int kind() { return 7; } };
template<class T> struct Qualified { int kind() { return 0; } };
template<class T> struct Qualified<const T &> {
    T value;
    int kind() { return 8; }
};
int main() {
    int number = 11;
    State<int &> reference;
    reference.value = &number;
    *reference.value = 19;
    Result<int> ordinary;
    Result<int *> pointer;
    Result<int &&> rvalue;
    Same<int *, int &> same;
    Same<int *, float &> different;
    Qualified<const int &> qualified;
    qualified.value = 23;
    return reference.kind() == 2 && number == 19 && ordinary.kind() == 1
        && pointer.kind() == 3 && rvalue.kind() == 4 && same.kind() == 7
        && different.kind() == 0 && qualified.kind() == 8
        && qualified.value == 23 ? 0 : 1;
}
