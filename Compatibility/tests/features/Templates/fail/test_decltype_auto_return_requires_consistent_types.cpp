// EXPECT_COMPILE_FAIL: 1
template<class T> decltype(auto) invalid(T value, bool select) {
    if (select) return value;
    return (value);
}
int main() { invalid(3, true); }
