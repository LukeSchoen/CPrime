template<class T> struct Outer {
 struct Base { virtual ~Base(){} virtual int get() const=0; };
 template<class F> struct Inner:Base { F value; Inner(F f):value(f){} int get() const override{return value;} };
 Base* p;
 template<class F> Outer(F f):p(new Inner<F>(f)){}
 ~Outer(){delete p;}
};
int main(){ Outer<int> o(3);return o.p->get()==3?0:1; }
