int calls;
struct Value { int number; Value(int n):number(n){} };
Value create(){++calls;return Value(42);}
int read(){static auto value=create();return ++value.number;}
int constant(){static const auto n=3; int a[n]; return sizeof(a)/sizeof(int);}
int main(){return read()!=43||read()!=44||calls!=1||constant()!=3;}
