// EXPECT_EXIT: 0
enum class Policy { now, later };
unsigned value() { return 17; }
template<class F, class... Args>
int choose(Policy policy, F function, Args... args) { return 1; }
template<class F, class... Args>
int choose(F function, Args... args) { return 2; }
template<class F, class... Args>
int reverse(F function, Args... args) { return 2; }
template<class F, class... Args>
int reverse(Policy policy, F function, Args... args) { return 1; }
template<class T, class... Args>
int conversion(double number, T value, Args... args) { return 1; }
template<class T, class... Args>
int conversion(T value, Args... args) { return 2; }
int main() {
    if (choose(Policy::now, value) != 1) return 1;
    if (reverse(Policy::later, value) != 1) return 2;
    if (choose(value) != 2) return 3;
    if (conversion(4, 5) != 2) return 4;
    if (conversion(4.0, 5) != 1) return 5;
    return 0;
}
