// EXPECT_COMPILE_FAIL: 1
// A class body instantiated by substitution is outside the immediate context.
template<class T> struct Broken { typedef typename T::missing type; };
template<class T> typename Broken<T>::type select(T);
int select(...);
int main() { return select(1); }
