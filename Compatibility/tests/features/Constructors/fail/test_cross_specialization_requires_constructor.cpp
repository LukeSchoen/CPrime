// EXPECT_COMPILE_FAIL: 1
// Template specializations are distinct classes; fields do not create a conversion.
template<class T> struct Components { T values[4]; };
int main() { Components<double> source = {}; Components<float> destination(source); }
