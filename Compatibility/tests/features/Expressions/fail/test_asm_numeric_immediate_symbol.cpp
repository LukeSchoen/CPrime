// EXPECT_COMPILE_FAIL: 1
int global;
int main() {
    asm volatile("" :: "n"(&global));
}
