template<class T> int select(T value) {
    if constexpr (sizeof(T) == 1) {
        if constexpr (sizeof(T) == 2) return value.missing;
        else return 3;
    } else if constexpr (sizeof(T) == sizeof(int)) return 5;
    else return value.missing;
}
int main() { return select(char(0)) != 3 || select(0) != 5; }
