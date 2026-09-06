struct Vector { bool a,b,c,d; Vector(bool x,bool y,bool z,bool w):a(x),b(y),c(z),d(w){} };
struct State { Vector value; State():value({true,false,true,false}){} };
int main(){State s;return s.value.a&&!s.value.b&&s.value.c&&!s.value.d?0:1;}
