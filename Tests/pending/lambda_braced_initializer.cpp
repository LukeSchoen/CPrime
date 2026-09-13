// Must compile and return zero.
struct Holder { int (*call)(); };
Holder holder = { []() { return 4; } };
int main() { return holder.call() != 4; }
