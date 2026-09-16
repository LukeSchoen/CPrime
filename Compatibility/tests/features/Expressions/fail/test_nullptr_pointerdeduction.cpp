// EXPECT_COMPILE_FAIL: 1
template<class T> int use(T*){return 1;} int main(){return use(nullptr);}
