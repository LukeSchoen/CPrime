// EXPECT_COMPILE_FAIL: 1
// EXPECT_COMPILE_ARGS: -Werror
int main() {
    static char text[2] = "longer";
    return text[0];
}
