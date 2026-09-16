// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
enum class Policy { now, later };
unsigned number() { return 17; }
int add(int left, int right) { return left + right; }
template<class Function, class... Args>
auto apply(Policy policy, Function function, Args... args)
  -> decltype(function(args...)) {
    return function(args...);
}
template<class Function, class... Args>
auto forward_apply(Function function, Args... args)
  -> decltype(apply(Policy::now, function, args...)) {
    return apply(Policy::now, function, args...);
}
int main() {
    if (apply(Policy::now, number) != 17) return 1;
    if (apply(Policy::now, add, 4, 9) != 13) return 2;
    if (forward_apply(number) != 17) return 3;
    return 0;
}
