// EXPECT_COMPILE_ARGS: -Werror
struct Property { unsigned length; };
namespace specialization {
struct Material {
  int bias;
  template<class T> int Get(const Property *property, T &out) const {
    out = T(7); return bias + 1;
  }
  template<class T> static int Convert(T value) { return 1; }
  template<class T> int Select(T value) const { return 2; }
  template<class T> int Select(T *value) const { return 3; }
};
template<> inline int Material::Get(const Property *property, bool &out) const {
  if (property->length < sizeof(bool)) return -1;
  out = true; return bias + 4;
}
template<> int Material::Convert(int value);
template<> int Material::Convert(int value) { return value + 4; }
template<> int Material::Select(int *value) const { return *value + 5; }
template<class T> int Function(T value) { return 6; }
template<class T> int Function(T *value) { return 7; }
template<> int Function(int *value) { return *value + 8; }
template<class A, class B> int Pair(A a, B b) { return 0; }
template<> int Pair<int>(int a, double b) { return a + int(b); }
template<class T> int Outside(T value) { return 0; }
struct Deferred { template<class T> int Get(T value) const; };
template<> int Deferred::Get(int value) const { return value + 1; }
template<class T> int Deferred::Get(T value) const { return 9; }
}
template<> int specialization::Outside(int value) { return value + 9; }
int main() {
  specialization::Material material;
  material.bias = 10;
  Property property; property.length = 1;
  bool flag = false;
  int value = 3;
  if (material.Get(&property, flag) != 14 || !flag) return 1;
  if (material.Get<bool>(&property, flag) != 14) return 2;
  if (material.Get(&property, value) != 11 || value != 7) return 3;
  value = 3;
  if (specialization::Material::Convert(value) != 7) return 4;
  if (material.Select(&value) != 8 || material.Select(value) != 2) return 5;
  if (specialization::Function(&value) != 11) return 6;
  if (specialization::Function(value) != 6) return 7;
  if (specialization::Pair(2, 3.0) != 5) return 8;
  if (specialization::Outside(3) != 12) return 9;
  specialization::Deferred deferred;
  if (deferred.Get(2) != 3 || deferred.Get('x') != 9) return 10;
  return 0;
}
