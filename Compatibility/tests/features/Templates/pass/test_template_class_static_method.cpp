template<typename T>
struct Box {
    static T value() {
        return (T)7;
    }
};

int main() {
    return Box<int>::value() - 7;
}
