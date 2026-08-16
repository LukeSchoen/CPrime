template<typename T>
struct identity;

template<>
struct identity<int> {
    static int value() {
        return 7;
    }
};

int main() {
    return identity<int>::value() - 7;
}
