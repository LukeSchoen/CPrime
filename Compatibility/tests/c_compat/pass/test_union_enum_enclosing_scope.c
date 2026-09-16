// EXPECT_EXIT: 0
union Owner { enum { COUNT = 3 } value; int items[COUNT]; };
int values[COUNT];
int main(void) { return sizeof(values) != 3 * sizeof(int); }
