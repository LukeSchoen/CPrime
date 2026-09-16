namespace sample {
struct Marker {int tag;};
template<class T> struct Outer {
  template<bool B> struct Inner {friend class Outer;typedef Marker marker_type;T value;Outer* owner;};
  typedef Inner<false> Item;
  Item make(T v){Item result;result.value=v;result.owner=this;return result;}
};
}
int main(){sample::Outer<int> o;auto item=o.make(7);return item.value!=7||item.owner!=&o;}
