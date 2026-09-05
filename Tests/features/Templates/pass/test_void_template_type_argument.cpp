// EXPECT_EXIT: 0
template<class T> struct Result { static const int kind = 1; };
template<> struct Result<void> { static const int kind = 2; };
template<class T> int kind() { return Result<T>::kind; }
int main() { return kind<void>() == 2 && Result<int>::kind == 1 ? 0 : 1; }
