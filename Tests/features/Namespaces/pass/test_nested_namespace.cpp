// EXPECT_EXIT: 0
// EXPECT_STDOUT: 24

#include <stdio.h>

namespace Outer {
namespace Inner {
int value = 9;
int plus(int n)
{
    return value + n;
}
}
}

int main()
{
    printf("%d\n", Outer::Inner::plus(15));
    return 0;
}
