// EXPECT_COMPILE_FAIL: 1
template<class T> struct Broken { typedef typename T::missing type; };
Broken<int> object;
int main() { return sizeof(object); }
