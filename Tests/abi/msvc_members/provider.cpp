#include "members.h"
// Observe the Microsoft constructor entry's returned receiver. Ordinary C++
// construction may discard RAX, while native optimized callers can reuse it.
extern "C" void* prime_constructor_entry(void*, int)
    __asm__("??0Prime_Counter@member_abi@@QEAA@H@Z");
namespace member_abi {
Native_Counter::Native_Counter(int v): value(v) {}
Native_Counter::~Native_Counter() { ++native_member_destructions; }
int Native_Counter::add(int v) const { return value + v; }
int Native_Counter::add(double v) const { return int(value + v); }
int Native_Counter::direct_virtual(int v) { return value + v; }
int Native_Counter::scale(short v) { return v * 3; }
int Prime_Counter::evaluate(int v) { return hidden(v) + adjust(3); }
int Outer_Names::Inner_Name::read() const { return value; }
int nested_argument(const Outer_Names::Inner_Name& value) { return value.value; }
int array_argument(const int (&value)[2][3]) { return value[1][2]; }
int scalar_arguments(char a, signed char b, unsigned char c, short d, unsigned short e,
                     int f, unsigned int g, long h, unsigned long i, long long j,
                     unsigned long long k, float l, double m, long double n, bool o, wchar_t p) {
  return a+b+c+d+e+f+g+h+i+j+k+int(l+m+n)+o+p;
}
int reference_argument(int&& value, const volatile int* pointer) { return value + *pointer; }
int callback_argument(int (*pointer)(int), int (&reference)(int)) { return pointer(2)+reference(3); }
int noexcept_callback(int (*pointer)(int) noexcept) { return pointer(4); }
template<class T> int native_template(T value) { return int(value)+sizeof(T); }
template int native_template<unsigned short>(unsigned short);
template<class... T> int empty_template() { return sizeof...(T); }
template int empty_template<>();
template<> int Native_Flag<true>::read() const { return 1; }
template<> int Native_Flag<false>::read() const { return 0; }
template<> int Native_Box<int, 2>::sum() const { return values[0]+values[1]; }
template<class T> int Template_Bridge::measure(T) const { return sizeof(T); }
template int Template_Bridge::measure<short>(short) const;
template int Template_Bridge::measure<double>(double) const;
Result native_result(int v) { Result result={v,v+1}; return result; }
Result Native_Counter::result(int v) const { Result result={value,value+v}; return result; }
Result Native_Counter::virtual_result(int v) { Result result={value+1,value+v}; return result; }
SmallResult Native_Counter::small(int v) const { SmallResult result={value+v}; return result; }
MediumResult Native_Counter::medium(int v) const { MediumResult result={value+v}; return result; }
SmallResult Native_Counter::static_small(int v) { SmallResult result={v}; return result; }
SmallResult Native_Counter::operator+(SmallResult v) const { SmallResult result={value+v.value}; return result; }
}
extern "C" int native_member_call(member_abi::Prime_Counter& value) {
  member_abi::Prime_Counter local(11);
  if (prime_constructor_entry(&local, 11) != &local) return -7;
  if (local.read() != 11 || !value || &value.self() != &value) return -1;
  if (value.compare(value, local) != 18) return -2;
  member_abi::Result free_result=member_abi::prime_result(12);
  member_abi::Result direct_result=value.result(4);
  member_abi::Result virtual_result=value.virtual_result(5);
  if (free_result.first!=12 || free_result.second!=13) return -3;
  if (direct_result.first!=7 || direct_result.second!=11) return -4;
  if (virtual_result.first!=8 || virtual_result.second!=12) return -5;
  member_abi::SmallResult small=value.small(3);
  member_abi::MediumResult medium=value.medium(4);
  member_abi::SmallResult staticSmall=member_abi::Prime_Counter::static_small(9);
  member_abi::SmallResult sum=value+small;
  if (small.value!=10 || medium.value!=11 || staticSmall.value!=9 || sum.value!=17) return -6;
  return value.member_abi::Prime_Counter::direct_virtual(3)
       + (value + 2) + member_abi::Prime_Counter::evaluate(4);
}
