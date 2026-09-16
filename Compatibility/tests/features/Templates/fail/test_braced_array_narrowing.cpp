// EXPECT_COMPILE_FAIL: 1
#include <stddef.h>
template<size_t N> int area(const unsigned char(&rows)[N]){return rows[0];}
int main(){return area({256});}
