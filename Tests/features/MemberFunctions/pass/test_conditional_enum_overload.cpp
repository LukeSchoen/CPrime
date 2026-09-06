enum Kind { A,B };enum Result { S,N };struct Node{};
struct Parser {
 void* make(){return 0;}
 Node* make(Kind,Result,const char*){return 0;}
 Node* make(Kind,Result,double){return 0;}
 Node* make(Kind,Result,Node* a=0,Node* b=0){return a;}
 Node* call(bool b,Node** args){return make(b?A:B,S,args[0]);}
};int main(){Parser p;Node n;Node* a[]={&n};return p.call(true,a)!=&n;}
