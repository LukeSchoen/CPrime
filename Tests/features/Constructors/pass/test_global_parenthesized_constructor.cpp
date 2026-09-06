int value=7;
int constructions;
struct Pointer {
  int* p;
  explicit Pointer(int* q):p(q){++constructions;}
};
Pointer pointer(&value);
Pointer* address(&pointer);
int number(9);
int main(){return *pointer.p!=7 || address!=&pointer || number!=9 || constructions!=1;}
