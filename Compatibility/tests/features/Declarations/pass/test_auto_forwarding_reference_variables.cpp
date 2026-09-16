#include <type_traits>
struct V{int n;};
int main(){const V v={3};auto&& ref=v;static_assert(std::is_same<decltype(ref),const V&>::value, "lvalue collapse");
 auto&& temp=4; static_assert(std::is_same<decltype(temp),int&&>::value, "temporary rvalue");
 const auto copy=v; static_assert(std::is_same<decltype(copy),const V>::value, "written const");
 auto plain=v;static_assert(std::is_same<decltype(plain),V>::value,"deduced value unqualified");
 auto&& rv=V{8}; const int& scalar=6; temp+=2; return ref.n!=3||temp!=6||copy.n!=3||plain.n!=3||rv.n!=8||scalar!=6;}
