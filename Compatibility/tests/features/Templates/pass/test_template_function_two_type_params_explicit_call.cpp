template<typename T, typename U>
T first(T a, U b) {
    return a;
}

int main() {
    return first<int, double>(11, 2.0) - 11;
}
