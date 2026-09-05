int read() { return 1; }
int read(int value) { return value + 2; }
struct Reader {
    static int read() { return 3; }
    static int read(double value) { return (int)value + 4; }
};
int main() {
    return read() != 1 || read(5) != 7
        || Reader::read() != 3 || Reader::read(5.0) != 9;
}
