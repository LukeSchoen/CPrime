int calls;
int next_value() { ++calls; return 4; }
struct Value { int first = next_value(); int second = first + 3; };
struct Outer { long long prefix[3]; Value member; };
Outer make() { return Outer{}; }
int main() {
    int result = Value{}.second;
    int nested = Outer{}.member.second;
    Outer returned = make();
    return result != 7 || nested != 7 || returned.member.second != 7 || calls != 3;
}
