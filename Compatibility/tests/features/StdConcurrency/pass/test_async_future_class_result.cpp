// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <future>
struct Result {
    int value;
    explicit Result(int input) : value(input) {}
};
Result produce() { return Result(41); }
int main() {
    std::future<Result> task = std::async(std::launch::async, produce);
    Result value = task.get();
    return value.value == 41 && !task.valid() ? 0 : 1;
}
