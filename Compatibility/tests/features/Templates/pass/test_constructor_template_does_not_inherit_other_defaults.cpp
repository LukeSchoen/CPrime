template<class T> struct Vec {
  T x, y, z;
  Vec(const T& a, const T& b = 0, const T& c = 0) : x(a), y(b), z(c) {}
  template<class U> Vec(const Vec<U>& o) : x(T(o.x)), y(T(o.y)), z(T(o.z)) {}
  template<class A, class B, class C> Vec(const A& a, const B& b, const C& c) : x(T(a)), y(T(b)), z(T(c)) {}
};
int main() { Vec<int> a(1, 2, 3); Vec<float> b(a); return b.x != 1 || b.y != 2 || b.z != 3; }
