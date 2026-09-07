#include <new>
#include <stdlib.h>
#include <string.h>
struct Member { long long size, capacity; void *data; Member():size(0),capacity(0),data(0){} };
struct Owner { Owner() { a=1; b=2; } int a,b,c; Member name; };
int main() { void *p=malloc(sizeof(Owner)); memset(p,0xa5,sizeof(Owner)); Owner *o=new(p) Owner(); int r=o->name.size!=0 || o->name.capacity!=0 || o->name.data!=0; free(p); return r; }
