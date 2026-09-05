// EXPECT_COMPILE_FAIL: 1
// EXPECT_SOURCES: ["strong_definition_other.cpp"]
// Matching code bytes do not permit two non-inline external definitions.
int repeated() { return 7; }
int main() { return repeated() != 7; }
