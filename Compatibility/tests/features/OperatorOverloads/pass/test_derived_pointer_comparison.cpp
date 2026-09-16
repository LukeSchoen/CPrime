struct Left{int a;};struct Right{int b;};struct Both:Left,Right{int c;};
int main(){Both object;Both* p=&object;const Right* base=&object;if(p!=base || base!=p)return 1;Both* null=0;const Right* zero=0;if(null!=zero || zero!=null)return 2;Both* const fixed=p;if(fixed!=base)return 3;return 0;}
