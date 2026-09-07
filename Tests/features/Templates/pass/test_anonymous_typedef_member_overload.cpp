typedef struct {int value;} Packet;
template<class T>struct Buffer {T item;void resize(int n){item.value=n;}void resize(int n,const T& v){item=v;item.value+=n;}};
int main(){Buffer<Packet> b;Packet p={5};b.resize(3,p);return b.item.value!=8;}
