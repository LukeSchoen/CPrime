// EXPECT_EXIT: 0
// EXPECT_STDOUT: 31

#include <stdio.h>

namespace Types {
struct Box {
    int value;
};

typedef Box Alias;

int read(Alias box)
{
    return box.value;
}
}

int main()
{
    Types::Box box;
    box.value = 31;
    printf("%d\n", Types::read(box));
    return 0;
}
