template<class T> struct Vec2 {
  T x, y;
  Vec2(T a, T b) : x(a), y(b) {}
};
template<class T> struct Vec3 {
  T x, y, z;
  Vec3(T a, T b, T c) : x(a), y(b), z(c) {}
};
template<class T> struct Vec4 {
  T x, y, z, w;
  Vec4(T a, T b, T c, T d) : x(a), y(b), z(c), w(d) {}
};
struct State { Vec4<bool> value; State():value({true,false,true,false}){} };
Vec2<Vec3<long long>> date(){return {{2026LL,9LL,6LL},{16LL,30LL,12LL}};}
int main(){State s;auto d=date();return s.value.x&&!s.value.y&&s.value.z&&!s.value.w && d.x.x==2026 && d.y.z==12?0:1;}
