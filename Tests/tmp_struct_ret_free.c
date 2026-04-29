struct V3 { int x; int y; };
typedef struct V3 V3;
V3 add(V3 a, V3 b) { V3 r={a.x+b.x,a.y+b.y}; return r; }
int main(void){ V3 a={1,2}, b={3,4}, c; c = add(a,b); return (c.x==4&&c.y==6)?0:1; }
