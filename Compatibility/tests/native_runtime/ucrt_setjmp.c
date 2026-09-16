#include <setjmp.h>
static jmp_buf target;
static volatile int calls;
static void nested(int count)
{
    ++calls;
    if (count) nested(count - 1);
    else longjmp(target, 42);
}
int main(void)
{
    int result = setjmp(target);
    if (!result) nested(5);
    if (result != 42 || calls != 6) return 1;
    result = setjmp(target);
    if (!result) longjmp(target, 0);
    return result != 1;
}
