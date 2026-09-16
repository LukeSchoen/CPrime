// EXPECT_EXIT: 0
struct B00 {}; struct B01 {}; struct B02 {}; struct B03 {}; struct B04 {};
struct B05 {}; struct B06 {}; struct B07 {}; struct B08 {}; struct B09 {};
struct B10 {}; struct B11 {}; struct B12 {}; struct B13 {}; struct B14 {};
struct B15 {}; struct B16 {}; struct B17 {}; struct B18 {}; struct B19 {};
struct B20 {}; struct B21 {}; struct B22 {}; struct B23 {}; struct B24 {};
struct B25 {}; struct B26 {}; struct B27 {}; struct B28 {}; struct B29 {};
struct B30 {}; struct B31 {}; struct B32 {};
struct Derived : B00, B01, B02, B03, B04, B05, B06, B07, B08, B09,
                 B10, B11, B12, B13, B14, B15, B16, B17, B18, B19,
                 B20, B21, B22, B23, B24, B25, B26, B27, B28, B29,
                 B30, B31, B32 {};
int main() { Derived value; return (B00 *)&value && (B32 *)&value ? 0 : 1; }
