// EXPECT_EXIT: 0
// A class declared in an inner block hides an ordinary name from an enclosing
// scope.
int main()
{
    static int same = 1;
    {
        struct same { int value; };
        same object;
        object.value = 2;
        if (object.value != 2)
            return 1;
    }
    return same != 1;
}
