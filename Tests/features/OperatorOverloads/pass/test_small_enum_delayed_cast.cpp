enum class Byte:unsigned char {Zero=0};
enum class Short:short {Zero=0};
int number(Byte value){return (int)value;}int number(Short value){return (int)value;}
Byte wrap(int value){return (Byte)value;}
int main(){volatile int value=511;Byte byte=(Byte)value;Short negative=(Short)(value-512);return number(byte)!=255 || number(wrap(value))!=255 || number(negative)!=-1;}
