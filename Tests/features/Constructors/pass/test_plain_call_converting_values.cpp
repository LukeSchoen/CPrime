struct Text {int size;Text(const char* s):size(0){while(s[size])++size;}};
int compare(Text a,Text b){return a.size-b.size;}
int main(){Text a("seven");return compare(a,"1234567")!=-2 || compare("1234567",a)!=2;}
