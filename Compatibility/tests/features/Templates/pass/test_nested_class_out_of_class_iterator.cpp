namespace Containers {
template<class T> struct Pool {
 T values[2];
 struct Iterator {
  Pool* pool;
  int index;
  Iterator(Pool* p, int i);
  Iterator& operator++();
  T& operator*();
  bool operator!=(Iterator const&) const;
 };
 Iterator begin() { return Iterator(this, 0); }
 Iterator end() { return Iterator(this, 2); }
};
static_assert(sizeof(Pool<int>) > 0, "complete before member definitions");
template<class T> Pool<T>::Iterator::Iterator(Pool<T>* p,int i):pool(p),index(i){}
template<class T> typename Pool<T>::Iterator& Pool<T>::Iterator::operator++(){++index;return *this;}
template<class T> T& Pool<T>::Iterator::operator*(){return pool->values[index];}
template<class T> bool Pool<T>::Iterator::operator!=(Iterator const& rhs)const{return index!=rhs.index;}
}
int main(){
 Containers::Pool<int> p;p.values[0]=3;p.values[1]=4;
 Containers::Pool<long> q;q.values[0]=8;q.values[1]=9;
 int sum=0;for(auto value:p)sum+=value;for(auto value:q)sum+=(int)value;
 return sum!=24;
}
