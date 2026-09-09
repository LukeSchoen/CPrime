template<class T> int calculate(T value, int add = 10, int scale = 2);
template<class U> int calculate(U input, int offset, int multiplier) {
    return (input + offset) * multiplier;
}
template<class T> int width(T value, int size = sizeof(T));
template<class U> int width(U input, int length) { return length; }
int main() {
    return calculate(3) != 26 || calculate(3, 4) != 14 || calculate(3, 4, 5) != 35
        || width(1) != sizeof(int) || width(1.0) != sizeof(double);
}
