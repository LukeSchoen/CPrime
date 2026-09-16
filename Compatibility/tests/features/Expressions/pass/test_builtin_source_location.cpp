// EXPECT_EXIT: 0
const char *source = __builtin_FILE();
const char *global_function = __builtin_FUNCTION();
#line 100
enum { line = __builtin_LINE() };
int main() {
    const char *function = __builtin_FUNCTION();
    if (line != 100 || !source[0]) return 1;
    return function[0] != 'm' || function[1] != 'a';
}
