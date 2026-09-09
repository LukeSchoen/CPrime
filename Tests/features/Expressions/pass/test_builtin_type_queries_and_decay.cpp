int function(int value) { return value; }
int data[9], calls;
enum Kind { item };
struct Record { int field; };
union Choice { int field; double other; };
int main() {
    Kind kind = item;
    if (__builtin_constant_p(function) || __builtin_constant_p(&data[0])) return 1;
    if (!__builtin_constant_p(6 * 7) || !__builtin_constant_p("literal")) return 2;
    if (__builtin_classify_type(function) != 5 || __builtin_classify_type(data) != 5) return 3;
    if (__builtin_classify_type(++calls) != 1 || calls) return 4;
    if (__builtin_classify_type(kind) != 3 || __builtin_classify_type(true) != 4) return 5;
    if (__builtin_classify_type(int[9]) != 14 || __builtin_classify_type(int&) != 6) return 6;
    if (__builtin_classify_type(Record) != 12 || __builtin_classify_type(Choice) != 13) return 7;
    if (__builtin_classify_type(double) != 8 || __builtin_classify_type(void) != 0) return 8;
    if (sizeof(({ data; })) != sizeof(int*) || ({ data; }) != data) return 9;
    return ({ function; })(42) != 42;
}
