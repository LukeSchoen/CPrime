// EXPECT_COMPILE_FAIL: 1
#include <stddef.h>
template<size_t R,size_t C> int area(const int(&rows)[R][C]){return R*C;}
int main(){return area({{1,2},{3,4,5}});}
