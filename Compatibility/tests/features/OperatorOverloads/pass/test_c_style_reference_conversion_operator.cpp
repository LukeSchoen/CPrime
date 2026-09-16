int calls;
struct Source {
    double padding;
    int value;
    Source() : padding(3.14), value(5) {}
    explicit operator int&() { ++calls; return value; }
};
int main() {
    Source source;
    int& reference = (int&)source;
    if (&reference != &source.value || reference != 5 || calls != 1) return 1;
    reference = 42;
    return source.value != 42 || source.padding != 3.14;
}
