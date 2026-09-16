int apply(int(*fn)(int),int value){return fn(value);}
int main(){
 auto next=[](int n){static int calls=0;return n+(++calls);};
 int(*p)(int)=next;
 if(next(10)!=11||p(10)!=12||apply(next,10)!=13)return 1;
 void(*set)(int&) = [](int& value){value=7;};int n=0;set(n);
 int&(*identity)(int&)=[](int& value)->int&{return value;};
 if(&identity(n)!=&n||n!=7)return 2;
 int(*mutable_call)(int)=[](int value)mutable{return value+5;};
 return mutable_call(3)!=8;
}
