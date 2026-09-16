int main(){int x=1;int*p=true?&x:nullptr;auto n=true?nullptr:nullptr;return *p!=1||n!=nullptr;}
