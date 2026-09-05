// EXPECT_EXIT: 0
int alive, destroyed, order;
struct Value {
  int id;
  Value(int n) : id(n) { ++alive; }
  ~Value() noexcept(false) { --alive; ++destroyed; order=order*10+id; if(id==3) throw 3; }
};
int fail() { throw 9; }
void consume(const Value &,const Value &) {}
int main() {
  try { (void)(Value(1), fail()); return 1; } catch(int n) { if(n!=9) return 2; }
  if(alive || destroyed!=1 || order!=1) return 3;
  try { consume(Value(2),Value(3)); return 4; } catch(int n) { if(n!=3) return 5; }
  if(alive || destroyed!=3 || order!=132) return 6;
  bool flag=false;
  try { (void)((flag ? Value(4).id : 0),fail()); } catch(int) {}
  if(alive || destroyed!=3) return 7;
  return 0;
}
