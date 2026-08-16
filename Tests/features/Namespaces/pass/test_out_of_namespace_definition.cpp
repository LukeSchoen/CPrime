// EXPECT_EXIT: 0
// EXPECT_STDOUT: 55

#include <stdio.h>

namespace Declared {
int base = 34;
int add(int n);
}

int Declared::add(int n)
{
    return Declared::base + n;
}

int main()
{
    printf("%d\n", Declared::add(21));
    return 0;
}
