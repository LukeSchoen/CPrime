// EXPECT_COMPILE_FAIL: 1
int ordinary(){return 3;}
constexpr int wrapper(){return ordinary();}
static_assert(wrapper()==3,"nonconstant call");
int main(){return 0;}

