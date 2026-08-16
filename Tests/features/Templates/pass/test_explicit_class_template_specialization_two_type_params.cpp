template<typename T, typename U>
struct PairValue {
    static int value() {
        return 1;
    }
};

template<>
struct PairValue<double, double> {
    static int value() {
        return 5;
    }
};

int main() {
    return PairValue<int, double>::value() + PairValue<double, double>::value() - 6;
}
