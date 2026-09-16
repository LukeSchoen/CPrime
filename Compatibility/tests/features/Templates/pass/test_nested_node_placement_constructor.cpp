#include <new>
#include <cstdlib>
struct Value { int n; };
template<class T> struct Owner {
  struct Node { T value; template<class V> Node(size_t tag,V&& v):value(v){} };
  template<class V> int create(V&& v) {
    void* memory=malloc(sizeof(Node));
    size_t tag=0;
    Node* node=new(memory) Node(tag,v);
    int result=node->value.n;node->~Node();free(memory);return result;
  }
};
int main(){Owner<Value> owner;Value v={7};return owner.create(v)!=7;}
