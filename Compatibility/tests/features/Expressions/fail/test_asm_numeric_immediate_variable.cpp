// EXPECT_COMPILE_FAIL: 1
int main(int argc, char **) {
    asm volatile("" :: "n"(argc));
}
