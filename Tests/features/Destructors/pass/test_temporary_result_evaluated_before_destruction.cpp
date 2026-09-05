// EXPECT_EXIT: 0
int alive;
struct Value {
  int n;
  Value(int input):n(input){++alive;}
  ~Value(){n=0;--alive;}
  operator bool() const {return n!=0 && alive>0;}
};
int scalar(){return Value(7).n;}
int main(){
  if(scalar()!=7 || alive) return 1;
  if(!Value(1)) return 2;
  if(alive) return 3;
  if(Value(0)) return 4;
  if(alive) return 5;
  int count=0;
  while(Value(count<3)) {if(alive) return 6; ++count;}
  if(count!=3 || alive) return 7;
  return 0;
}
