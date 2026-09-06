#include <memory>
#include <type_traits>
template<class T,class Alloc=std::allocator<T>> struct Container {
  struct Node {T value;};
  typedef typename Alloc::template rebind<Node>::other NodeAllocator;
  static_assert(std::is_same<typename NodeAllocator::value_type,Node>::value,"rebind must replace the allocated type");
  NodeAllocator allocator;
  int run(){Node* p=allocator.allocate(1);new(p)Node();p->value=7;int n=p->value;p->~Node();allocator.deallocate(p,1);return n;}
};
int main(){Container<int> c;return c.run()!=7;}
