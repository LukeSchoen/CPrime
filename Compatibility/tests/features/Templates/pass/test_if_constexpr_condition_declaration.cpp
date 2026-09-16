constexpr int number() { return 3; }
int main() {
    if constexpr (constexpr int value = 3) {
        if (value != 3) return 1;
    } else return 2;
    if constexpr (const int value = number()) {
        if (value != 3) return 3;
    } else return 4;
    if constexpr (constexpr int value = 0) return 5;
    else if (value != 0) return 6;
}
