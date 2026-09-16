// EXPECT_EXIT: 0
// EXPECT_SOURCES: ["template_member_odr_other.cpp"]
#include "template_member_odr.h"
typedef int (*Function)(int);
Function other_identity();
int other_assignment();
int main() {
    SharedList<int> a, b;
    b.value = 4; a = b;
    if (other_identity() != &SharedList<int>::identity) return 1;
    return a.value + other_assignment() != 7;
}
