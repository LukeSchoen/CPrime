template<class T> struct Box {
    T value;
    explicit Box(const T& input) : value(input) {}
    T get() const { return value; }
};
template<class To> To convert(int input) {
    return To(input);
}
int main() {
    if (convert<Box<double>>(15).get() != 15.0) return 1;
    return convert<Box<long long>>(23).get() != 23;
}
