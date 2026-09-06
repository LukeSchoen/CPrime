#include <new>
#include <stddef.h>
struct Heap {
 void* operator new(size_t,const std::nothrow_t&) throw();
 void operator delete(void*) noexcept;
};
const std::nothrow_t* identity(const std::nothrow_t& value){return &value;}
int main(){std::nothrow_t local;return !identity(local)||identity(std::nothrow)!=&std::nothrow;}
