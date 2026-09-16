struct State {
  int* p; int value;
  State():p(&value),value(0){}
  void share(State& other){p=other.p;}
  int get()const{return *p;}
};
struct Input:State{};
struct Output:State{};
struct Both:Input,Output{
  Both(){Input::share((Output&)*this);}
  int read(){return Input::get();}
};
int main(){Both b;((Output&)b).value=9;return b.read()!=9;}
