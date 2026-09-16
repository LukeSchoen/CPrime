#include <algorithm>
#include <ctype.h>
int main(){char s[]="abc";std::transform(s,s+3,s,::toupper);return s[0]!='A';}
