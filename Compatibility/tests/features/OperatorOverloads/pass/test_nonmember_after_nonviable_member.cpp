struct Value { int n; Value& operator<<(const char*) { n=99;return *this; } };
struct Manip { int n; };
Value& operator<<(Value& v,Manip m){v.n=m.n;return v;}
int main(){Value v;v.n=0;Manip m={7};v << m;return v.n!=7;}
