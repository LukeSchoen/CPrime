// EXPECT_EXIT: 0
// EXPECT_STDOUT: implicit 7
// A call to a function that neither a visible declaration nor argument-dependent
// lookup provides publishes the same implicit external declaration the C
// frontend tolerates, so CL sources that call printf, clock or getchar without
// the matching system header still compile instead of failing overload
// resolution.  The second call reuses the symbol the first one published.
int main()
{
    int first = printf("implicit %d", 7);
    int second = printf("\n");
    return first == 10 && second == 1 ? 0 : 1;
}
