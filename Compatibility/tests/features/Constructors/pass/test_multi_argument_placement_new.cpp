#include <stddef.h>
struct Tag {int value;};
int allocated, constructed;
void* operator new(size_t bytes,Tag tag,void* address){allocated+=(int)bytes+tag.value;return address;}
struct Item {int value;Item(int v):value(v){++constructed;}};
int main(){union Storage {int alignment;unsigned char data[sizeof(Item)];} storage;Tag tag={3};Item* p=new(tag,storage.data)Item(7);int result=p->value!=7||constructed!=1||allocated!=(int)sizeof(Item)+3;p->~Item();return result;}
