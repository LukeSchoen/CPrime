struct Context { int n; };
struct Stack { int* p; };
struct Equal { bool operator()(int a,int b) const {return a==b;} };
struct Node {
  template<class Comp> static bool compare(Node* a,Node* b,const Context& c,const Stack& s,const Comp& comp) {return comp(c.n,*s.p);}
  bool run(const Context& c,const Stack& s){return compare(this,this,c,s,Equal());}
};
int main(){int n=4;Context c={4};Stack s={&n};Node a;return !a.run(c,s);}
