// EXPECT_EXIT: 0
// EXPECT_STDOUT: implicit 7
// Retain CPC's C compatibility extension for an implicit int() declaration.
int main()
{
    int first = printf("implicit %d", 7);
    int second = printf("\n");
    return first == 10 && second == 1 ? 0 : 1;
}
