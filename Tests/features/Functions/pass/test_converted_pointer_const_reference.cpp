struct Left{int a;};
struct Base{int n;};
struct Derived:Left,Base{};
Base* read(Base* const& p){return p;}
const Base* read_const(const Base* const& p){return p;}
int main(){Derived d;d.n=7;Derived* p=&d;return read(p)!=static_cast<Base*>(p)
  || read_const(p)->n!=7 || read(nullptr)!=nullptr || p!=&d;}
