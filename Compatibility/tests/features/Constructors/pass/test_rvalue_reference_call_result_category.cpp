// EXPECT_EXIT: 0
int copies,moves;
struct Value {
  int n;
  Value(int input):n(input){}
  Value(const Value &other):n(other.n){++copies;}
  Value(Value &&other):n(other.n){++moves;other.n=-1;}
  Value&& get(){return static_cast<Value&&>(*this);}
};
Value&& move_value(Value &input){return static_cast<Value&&>(input);}
int consume(Value input){return input.n;}
int main(){
  Value first(7); Value second(move_value(first));
  if(first.n!=-1 || second.n!=7 || moves!=1 || copies) return 1;
  Value&& named=move_value(second); Value third(named);
  if(second.n!=7 || third.n!=7 || moves!=1 || copies!=1) return 2;
  if(consume(third.get())!=7 || third.n!=-1 || moves!=2 || copies!=1) return 3;
  return 0;
}
