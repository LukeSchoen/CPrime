// EXPECT_COMPILE_ARGS: -Werror
#include <string.h>
#include <type_traits>
int same(const char *a,const char *b) { return strcmp(a,b)==0 ? 1 : 2; }
template<class T> int same(T,T) { return 3; }
int narrow(const char (&)[5]) { return 4; }
int narrow(char (&)[5]) { return 5; }
int wide(const wchar_t (&)[5]) { return 6; }
int wide(wchar_t (&)[5]) { return 7; }
typedef const char (&NarrowLiteral)[5];
typedef const wchar_t (&WideLiteral)[5];
int main() {
 char text[]="same";
 static_assert(std::is_same<decltype("same"),NarrowLiteral>::value,"literal array is const");
 static_assert(std::is_same<decltype(L"wide"),WideLiteral>::value,"wide literal array is const");
 if(same("same",text)!=1) return 1;
 if(narrow("same")!=4 || narrow(text)!=5 || wide(L"wide")!=6) return 2;
 text[0]='S';
 return text[0]!='S';
}
