template<class T> int target(T) { return T::missing; }
template<class T> int selected(T value) {
    if constexpr (sizeof(T) == 0) return target(value);
    return 7;
}
inline int unused() { return selected(1); }
int main() { return selected(1) != 7; }
