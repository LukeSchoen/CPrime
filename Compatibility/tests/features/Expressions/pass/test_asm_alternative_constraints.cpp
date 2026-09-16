// EXPECT_EXIT: 0
// An asm operand constraint may list comma-separated alternatives.  The
// allocator uses the first viable alternative.
int main()
{
    int value = 41;
    __asm volatile ("" : "+m,r" (value));
    return value != 41;
}
