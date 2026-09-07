struct Text {int value;};
struct Command {
 bool execute(int* value,const Text& a,const Text& b){*value=a.value+b.value;return true;}
 bool run(int* value,const Text& a,const Text& b){
  bool(Command::*function)(int*,const Text&,const Text&)=&Command::execute;
  return (this->*function)(value,a,b);
 }
};int main(){Command c;Text a={3},b={4};int n=0;return !c.run(&n,a,b)||n!=7;}
