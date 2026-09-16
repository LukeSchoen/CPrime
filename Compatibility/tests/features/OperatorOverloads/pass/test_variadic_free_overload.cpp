#include <stdarg.h>
int node(const char*){return 1;}
int node(const char*,const char*,...){return 2;}
int node(const void*,const char*,...){return 3;}
int choose(int){return 4;}
int choose(...){return 5;}
int main(){return node("id","format",3)!=2||node((void*)0,"format",3)!=3||choose(7)!=4||choose("text")!=5;}
