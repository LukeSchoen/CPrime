static int constructed, destroyed, sum;
struct Value { explicit Value(int n=0):n(n){++constructed;} ~Value(){++destroyed;} int n; };
Value operator+(const char*,const Value&v){return Value(v.n+1);}
struct Sink { explicit Sink(const Value&v){sum+=v.n;} };
Value &identity(Value&v){return v;}
namespace test { struct Other { explicit Other(int n){sum+=n;} }; }
int main(){
  Sink("samples"+Value(4));
  if(sum!=5 || constructed!=destroyed)return 1;
  Sink(Value(2));
  if(sum!=7 || constructed!=destroyed)return 2;
  test::Other(3);
  using Alias=test::Other;
  Alias{4};
  (void)int(8+9);
  if(sum!=14)return 3;
  {
    Value (name);
    if(name.n!=0)return 4;
    Value (&reference)=name;
    Value (*pointer)=&name;
    if(&reference!=pointer)return 5;
    Sink(identity(name));
  }
  return constructed!=destroyed?6:0;
}
