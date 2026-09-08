// EXPECT_EXIT: 0
int value;
template<class T> void record(T n) { value = n; }
template<void (*F)(int)> struct Dispatch { static void run(int n) { F(n); } };
template<void (*F)(char)> void run_char() { F(17); }
void overloaded(int n) { value = n + 1; }
void overloaded(double);
int main() {
    Dispatch<record>::run(13);
    if (value != 13) return 1;
    run_char<record>();
    if (value != 17) return 2;
    Dispatch<overloaded>::run(19);
    return value != 20;
}
