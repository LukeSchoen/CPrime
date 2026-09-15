struct Condition {
    constexpr explicit operator bool() const { return true; }
};
int main() {
    if constexpr (Condition{}) return 0;
    else return 1;
}
