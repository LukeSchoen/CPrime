// EXPECT_COMPILE_FAIL: 1
namespace source {
    int choose(int);
    int choose(double);
}
namespace imported { using source::choose; }
int (*function)(const char *) = imported::choose;
