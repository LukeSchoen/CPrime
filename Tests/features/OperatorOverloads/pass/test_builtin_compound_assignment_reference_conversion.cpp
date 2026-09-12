// EXPECT_EXIT: 0
int conversions;
struct Counter {
    int value;
    Counter(int v) : value(v) {}
    operator int&() { ++conversions; return value; }
};
int main() {
    Counter counter(5);
    counter += 3;
    if (counter.value != 8 || conversions != 1) return 1;
    counter *= 2;
    counter -= 6;
    counter ^= 1;
    if (counter.value != (10 ^ 1) || conversions != 4) return 2;
    int& result = (counter += 1);
    result = 7;
    return counter.value != 7 || conversions != 5;
}
