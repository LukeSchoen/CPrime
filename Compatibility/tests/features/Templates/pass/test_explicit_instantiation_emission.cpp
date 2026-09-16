// EXPECT_EXIT: 0
// EXPECT_SOURCES: ["explicit_instantiation_provider.cpp"]
template<class T> T multiply(T);
int main() { return multiply<int>(6) != 42; }
