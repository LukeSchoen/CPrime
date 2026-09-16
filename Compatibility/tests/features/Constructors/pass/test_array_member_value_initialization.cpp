struct Value { int n; Value():n(17){} };
struct Group { int scalars[4]; int* pointers[3]; Value objects[2]; Group():scalars(),pointers(),objects(){} };
int main(){Group g;for(int i=0;i<4;++i)if(g.scalars[i])return 1;for(int i=0;i<3;++i)if(g.pointers[i])return 2;return g.objects[0].n!=17||g.objects[1].n!=17;}
