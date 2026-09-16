template<class T, class U> struct Pair {
    static int value();
    static int unused();
};
template<class T, class U> int Pair<T, U>::value() { return sizeof(T) * 10 + sizeof(U); }
template<class T, class U> int Pair<T, U>::unused() { return T::missing; }

template<class T> struct Other { static int value(); };
template<class T> int Other<T>::value() { return sizeof(T) + 100; }

struct Ordinary { static int value() { return 7; } };

int main() {
    // Instantiate owners in a different order from their declarations.
    if (Other<char>::value() != 101) return 1;
    if (Pair<int, char>::value() != 41) return 2;
    if (Pair<char, int>::value() != 14) return 3;
    if (Other<int>::value() != 104) return 4;
    if (Ordinary::value() != 7) return 5;
    return Pair<int, char>::value() != 41;
}
