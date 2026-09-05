// EXPECT_COMPILE_FAIL: 1
template<class T> void require_same(T*, T*) {}
int main() {
    int value = 0;
    const int* constant = &value;
    require_same(&value, constant);
}
