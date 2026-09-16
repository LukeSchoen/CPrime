struct A{int v;int f(){return 1;}};int main(){auto n=nullptr;int A::* p=n;int(A::*f)()=n;return p!=nullptr||f!=nullptr;}
