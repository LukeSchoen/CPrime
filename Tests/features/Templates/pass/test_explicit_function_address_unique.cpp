// EXPECT_EXIT: 0
template<class T, class U> U pick(T, U);
template<class T> int pick(T value) { return value + 3; }
template<class F> int invoke(F function) { return function(4); }
int main() { return invoke(pick<int>) != 7; }
