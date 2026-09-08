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
int main() { return sizeof(object) != 1048576; }
