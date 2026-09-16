int calls;
int value(int n) { ++calls; return n; }
const int first = value(7);
const double second = value(9) + .5;
int stored = 12;
int* address(){ ++calls; return &stored; }
int* const pointer = address();
struct Observe { int n; Observe(): n(first + int(second) + *pointer) {} } observer;
int main(){ return calls != 3 || first != 7 || second != 9.5 || pointer != &stored || observer.n != 28; }
