struct Condition {
    int value;
    constexpr explicit operator bool() const { return this->value != 0; }
};
constexpr Condition disabled{0}, enabled{3};
int main() {
    if constexpr (disabled) return 1;
    if constexpr (enabled) return 0;
    else return 2;
}
