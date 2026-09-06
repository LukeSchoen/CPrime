struct Target { int value; };
struct Inner { Target* p; Target* operator->() const { return p; } };
struct Outer { Target* p; Inner operator->() const { return {p}; } };
int main() { Target t = {7}; const Outer p = {&t}; p->value += 2; return p->value != 9; }
