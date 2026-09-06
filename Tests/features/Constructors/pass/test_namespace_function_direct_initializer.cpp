#include <utility>
struct Value {
  int n;
  Value(int x):n(x){}
  Value(const Value& x):n(x.n){}
  Value(Value&& x):n(x.n){x.n=0;}
  Value take()&& { Value result(std::move(*this));return result; }
};
int main(){Value v(7);Value w(std::move(v));Value x=std::move(w).take();return v.n || w.n || x.n!=7;}
