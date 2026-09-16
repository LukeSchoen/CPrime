// EXPECT_EXIT: 0
template<int N> struct Size { char data[N]; };
struct Record { double value; };
int main() {
    Size<__alignof__(Record)> a;
    Size<alignof(int)> b;
    return sizeof(a) != alignof(Record) || sizeof(b) != alignof(int);
}
