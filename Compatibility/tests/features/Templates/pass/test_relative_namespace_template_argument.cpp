namespace library {
namespace impl { namespace {
struct Object { int n; };
template<class T> struct Holder {
  typedef void (*Cleanup)(T*);
  T* data;
  Cleanup cleanup;
  Holder(T* p,Cleanup c):data(p),cleanup(c){}
  ~Holder(){cleanup(data);}
};
void destroy(Object* p){p->n=9;}
}}
int run() {
  using impl::Holder;
  impl::Object o = {1};
  { Holder<impl::Object> impl(&o, impl::destroy); }
  return o.n != 9;
}
}
int main(){return library::run();}
