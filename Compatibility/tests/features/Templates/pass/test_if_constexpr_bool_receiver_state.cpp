struct Condition {
    int value;
    constexpr explicit operator bool() const { return value != 0; }
};
int main() {
    if constexpr (Condition{0}) return 1;
    if constexpr (Condition{3}) return 0;
    else return 2;
}
