// EXPECT_LINK_FAIL: 1
// Runner fixture: a compilation error must not satisfy a link expectation.
int main() { return undeclared_name; }
