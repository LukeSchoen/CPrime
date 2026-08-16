// EXPECT_EXIT: 0
// EXPECT_STDOUT: 42 7

#include <stdio.h>

namespace Alpha {
int value = 42;
int get()
{
    return 7;
}
}

int main()
{
    printf("%d %d\n", Alpha::value, Alpha::get());
    return 0;
}
