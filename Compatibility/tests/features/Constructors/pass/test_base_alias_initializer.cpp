struct Base { int value; Base(int n):value(n){} };
using Parent=Base;
struct Derived:Parent { Derived():Parent(7){} };
int main(){Derived d;return d.value==7?0:1;}
