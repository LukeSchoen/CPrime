#include <new>
#include <utility>
struct Value {
  int number;
  Value(int n) : number(n) {}
  Value(const Value& other) : number(other.number+1) {}
  Value(Value&& other) : number(other.number+10) { other.number=0; }
};
Value& lvalue(Value& value) { return value; }
Value&& xvalue(Value& value) { return static_cast<Value&&>(value); }
int main() {
  union Storage { char bytes[sizeof(Value)]; long long alignment; } a,b,c;
  Value source(7);
  Value* copied=new(a.bytes) Value(lvalue(source));
  if (copied->number!=8 || source.number!=7) return 1;
  Value* moved=new(b.bytes) Value(xvalue(source));
  if (moved->number!=17 || source.number!=0) return 2;
  Value last(11);
  Value* forwarded=new(c.bytes) Value(std::move(last));
  return forwarded->number!=21 || last.number!=0;
}
