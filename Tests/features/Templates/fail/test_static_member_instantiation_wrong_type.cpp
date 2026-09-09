// EXPECT_COMPILE_FAIL: 1
template<class T> struct Value { static int number; };
template<class T> int Value<T>::number = 3;
template double Value<int>::number;
