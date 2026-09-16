enum Mode { First, Second };
struct Text { Text(int) {} };
struct Number { Number(int) {} };
Text operator+(Text,Text){return Text(0);}
Number operator+(Number,Number){return Number(0);}
enum Flags { A=1,B=2 };
Flags operator|(Flags a,Flags b){return Flags(int(a)|int(b));}
int main(){Mode m=First;return m+1!=1 || int(A|B)!=3;}
