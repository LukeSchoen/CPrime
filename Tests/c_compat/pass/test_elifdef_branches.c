// EXPECT_EXIT: 0
#define PRESENT
#if 0
#elifdef ABSENT
#error selected absent macro
#elifndef PRESENT
#error selected defined macro
#elifdef PRESENT
#define FIRST 3
#else
#error missed defined macro
#endif
#if 0
#elifndef ABSENT
#define SECOND 4
#endif
#if 1
#elifdef invalid * syntax
#elifndef !
#endif
#if 0
#if 0
#elifndef ABSENT
#error nested skipped branch selected
#endif
#elifndef ABSENT
#define THIRD 5
#endif
int main(void) { return FIRST + SECOND + THIRD != 12; }
