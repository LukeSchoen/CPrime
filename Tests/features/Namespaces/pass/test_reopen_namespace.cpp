// EXPECT_EXIT: 0
// EXPECT_STDOUT: 13

#include <stdio.h>

namespace Reopen {
int a = 5;
}

namespace Reopen {
int b = 8;
int sum()
{
    return a + b;
}
}

int main()
{
    printf("%d\n", Reopen::sum());
    return 0;
}
