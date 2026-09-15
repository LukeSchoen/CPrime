// EXPECT_COMPILE_FAIL: 1
// C++ cannot invent a declaration when ordinary lookup and ADL find none.
int main()
{
    int first = printf("implicit %d", 7);
    int second = printf("\n");
    return first == 10 && second == 1 ? 0 : 1;
}
