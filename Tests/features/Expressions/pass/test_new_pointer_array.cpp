struct Value { int n; };
int main(){Value v={7};Value** a=new Value*[3]();for(int i=0;i<3;++i)if(a[i])return 1;a[1]=&v;int result=a[1]->n;delete[] a;int** b=new int*(nullptr);if(*b)return 2;delete b;return result!=7;}
