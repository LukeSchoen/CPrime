// EXPECT_COMPILE_FAIL: 1
const char *text = R"bad delimiter(x)bad delimiter";
int main() {}
