int destroyed, created;
struct Check {int n;Check(int x):n(x){++created;}~Check(){++destroyed;}explicit operator bool()const{return n>0;}};
int main(){const unsigned char* text=(const unsigned char*)"abc";int chars=0;while(unsigned char c=*text++)chars+=c;
int n=3;while(Check c=--n){if(c.n==2)continue;}if(created!=3||destroyed!=3)return 1;
n=5;while(Check c=--n){break;}return chars!=294||created!=4||destroyed!=4;}
