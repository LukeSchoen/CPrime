// EXPECT_COMPILE_FAIL: 1
int accept(int (&&array)[2]) { return array[0]; }
int main() { int array[2] = {3, 4}; return accept(array); }
