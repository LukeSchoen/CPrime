template<typename T>
struct identity;

template<>
struct identity<double> {
    static double value() {
        return 2.5;
    }
};

int main() {
    double v = identity<double>::value();
    return v == 2.5 ? 0 : 1;
}
