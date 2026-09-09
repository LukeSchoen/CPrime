// EXPECT_EXIT: 0
template<int Number> int read(void);
template<int Value> int read() { return Value; }
template<class T> int size(void) { return sizeof(T); }
template<class T> int pointer(void *value) { return value != 0; }
int main() {
    int object;
    return read<7>() != 7 || size<int>() != sizeof(int)
        || pointer<int>(&object) != 1;
}
