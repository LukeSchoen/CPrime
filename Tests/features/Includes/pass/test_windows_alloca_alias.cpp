#include <malloc.h>
#ifndef alloca
#define alloca _alloca
#endif
int main(){int* a=(int*)alloca(16);int* b=(int*)_alloca(16);a[0]=7;b[0]=8;return a[0]+b[0]!=15;}
