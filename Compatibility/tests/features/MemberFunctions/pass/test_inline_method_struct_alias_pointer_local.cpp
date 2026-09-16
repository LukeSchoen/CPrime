// EXPECT_EXIT: 0
struct E { int a; int b; };
struct M { E* find(int){return (E*)1;} E* end(){return (E*)1;} };
struct T {
  M m;
  int f(int k){
    E* it = m.find(k);
    if(it != m.end())
      return it->b;
    return 0;
  }
};
int main(){T t; return t.f(1);} 
