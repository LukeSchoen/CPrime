template<bool Function(double)> bool invoke(double value) { return Function(value); }
struct Handler {
    template<class T> static bool run(T value) { return invoke<accept<T> >(value); }
private:
    template<class T> static bool accept(T value) { return value > 0; }
};
int main() { return !Handler::run<double>(1) || Handler::run<double>(-1); }
