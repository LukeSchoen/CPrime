struct Value {
    int zero;
    int first = 4;
    int second = zero + first;
};
int main() {
    int first = 99;
    Value runtime{};
    constexpr Value value{};
    static_assert(value.second == 4);
    return value.zero != 0 || value.first != 4 || value.second != 4
        || runtime.second != 4 || first != 99;
}
