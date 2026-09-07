#include <functional>
int check(int n){static const int values[]={2,4};auto fn=[](int x){for(auto v:values)if(v==x)return true;return false;};return fn(n);}
int counter(){static int value=3;auto fn=[](){++value;return value;};int(*pointer)()=fn;int first=fn();return first+pointer()+value;}
int shadow(){static int value=9;auto fn=[](){int value=2;return value;};return fn()+value;}
int main(){return !check(2)||check(3)||counter()!=14||shadow()!=11;}
