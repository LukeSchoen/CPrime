int calls=0;void act(){++calls;}
int main(){auto f=[]()->auto{return act();};auto n=[]()->auto{return 9LL;};int value=3;auto ref=[&]()->auto&{return value;};auto ptr=[&]()->auto*{return &value;};f();ref()=7;return calls!=1 || n()!=9 || sizeof(n())!=8 || ptr()!=&value || value!=7;}
