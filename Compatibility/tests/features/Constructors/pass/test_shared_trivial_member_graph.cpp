struct Leaf { char value; };
template<class T> struct Pair { T left, right; };
typedef Pair<Leaf> L1;
typedef Pair<L1> L2;
typedef Pair<L2> L3;
typedef Pair<L3> L4;
typedef Pair<L4> L5;
typedef Pair<L5> L6;
typedef Pair<L6> L7;
typedef Pair<L7> L8;
typedef Pair<L8> L9;
typedef Pair<L9> L10;
typedef Pair<L10> L11;
typedef Pair<L11> L12;
typedef Pair<L12> L13;
typedef Pair<L13> L14;
typedef Pair<L14> L15;
typedef Pair<L15> L16;
typedef Pair<L16> L17;
typedef Pair<L17> L18;
typedef Pair<L18> L19;
typedef Pair<L19> L20;
void compile_local_lifetime() { L20 local; }
// Exercise local lifetime analysis without running a large stack allocation.
static L20 object;
// More distinct subobjects than the initial query table holds exercise growth
// while a repeated member graph exercises reuse. All copies remain trivial.
template<int N> struct Tagged { int value; };
#define MEMBERS(X) X(0) X(1) X(2) X(3) X(4) X(5) X(6) X(7) X(8) X(9) \
    X(10) X(11) X(12) X(13) X(14) X(15) X(16) X(17) X(18) X(19) \
    X(20) X(21) X(22) X(23) X(24) X(25) X(26) X(27) X(28) X(29) \
    X(30) X(31) X(32) X(33) X(34) X(35) X(36) X(37) X(38) X(39)
#define FIELD(N) Tagged<N> field##N;
struct Wide { MEMBERS(FIELD) };
#undef FIELD
int main() {
    Wide first{};
    first.field0.value = 7;
    first.field39.value = 11;
    const Wide second(first);
    Wide third{};
    third = second;
    return sizeof(object) != 1048576 || third.field0.value != 7 || third.field39.value != 11;
}
