struct Index {
    long long operator[](const long long &value) { return value + 1; }
};
double read(const double &value) { return value; }
long long read_integer(const long long &value) { return value; }
int main() {
    Index index;
    int number = 41;
    signed char small = -7;
    if (index[number] != 42 || index[2 + 3] != 6) return 1;
    if (read(number) != 41.0 || read_integer(small) != -7) return 2;
    if (number != 41 || small != -7) return 3;
    return 0;
}
