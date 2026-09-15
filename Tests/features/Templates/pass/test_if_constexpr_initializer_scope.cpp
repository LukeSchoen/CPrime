int calls;
int source() { ++calls; return 9; }
template<class T> int select() {
    if constexpr (int value = source(); sizeof(T) == 1) return value;
    else return value + 1;
}
int alive, destroyed;
struct Guard {
    Guard() { ++alive; }
    ~Guard() { --alive; ++destroyed; }
};
template<class T> int lifetime() {
    if constexpr (Guard guard; sizeof(T) == 1) {
        if (alive != 1) return 1;
    } else {
        if (alive != 1) return 2;
    }
    return alive != 0;
}
int main() {
    if (select<char>() != 9 || select<int>() != 10 || calls != 2) return 1;
    if (lifetime<char>() || lifetime<int>() || destroyed != 2) return 2;
    if constexpr (++calls; true) { if (calls != 3) return 3; }
    if constexpr (; false) return 4;
}
