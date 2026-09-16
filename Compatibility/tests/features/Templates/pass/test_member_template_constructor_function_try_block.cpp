struct Collector {
    template<class T>
    Collector(T) try {
    } catch (...) {
    }
};

int main() {
    Collector value(1);
    (void)value;
    return 0;
}
