#include "template_member_odr.h"
typedef int (*Function)(int);
Function other_identity() { return &SharedList<int>::identity; }
int other_assignment() {
    SharedList<int> a, b;
    b.value = 3; a = b;
    return a.value;
}
