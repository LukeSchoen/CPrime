template<typename T, typename U>
struct PairOps {
    static T first(T a, U b) {
        return a;
    }
};

int main() {
    return PairOps<int, double>::first(9, 2.0) - 9;
}
