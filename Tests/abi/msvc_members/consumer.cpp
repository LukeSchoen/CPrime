#include "members.h"
extern "C" {
int native_member_destructions;
int prime_member_destructions;
}
namespace member_abi {
int constructor_calls;
int constructor_side_effect() { ++constructor_calls; return -41; }
Prime_Counter::Prime_Counter(int v): value(v) { constructor_side_effect(); }
Prime_Counter::~Prime_Counter() { ++prime_member_destructions; }
int Prime_Counter::hidden(int v) { return v + 1; }
int Prime_Counter::adjust(short v) { return v * 2; }
int Prime_Counter::read() const { return value; }
int Prime_Counter::direct_virtual(int v) { return value + v; }
int Prime_Counter::operator+(int v) const { return value + v; }
Prime_Counter::operator bool() const { return value != 0; }
const Prime_Counter& Prime_Counter::self() const { return *this; }
int Prime_Counter::compare(const Prime_Counter& a, const Prime_Counter& b) const {
  return a.value + b.value;
}
Result prime_result(int v) { Result result={v,v+1}; return result; }
Result Prime_Counter::result(int v) const { Result result={value,value+v}; return result; }
Result Prime_Counter::virtual_result(int v) { Result result={value+1,value+v}; return result; }
SmallResult Prime_Counter::small(int v) const { SmallResult result={value+v}; return result; }
MediumResult Prime_Counter::medium(int v) const { MediumResult result={value+v}; return result; }
SmallResult Prime_Counter::static_small(int v) { SmallResult result={v}; return result; }
SmallResult Prime_Counter::operator+(SmallResult v) const { SmallResult result={value+v.value}; return result; }
}
int callback(int value) { return value*3; }
int safe_callback(int value) noexcept { return value*5; }
int main() {
  {
    member_abi::Native_Counter native(5);
    member_abi::Prime_Counter prime(7);
    member_abi::Outer_Names::Inner_Name nested = {9};
    int array[2][3] = {{1,2,3},{4,5,6}};
    int reference=4;
    volatile int pointee=5;
    member_abi::Native_Box<int,2> box={{3,5}};
    member_abi::Template_Bridge bridge;
    if (member_abi::nested_argument(nested)!=9 || member_abi::array_argument(array)!=6) return 5;
    if (member_abi::scalar_arguments(1,1,1,1,1,1,1,1,1,1,1,1,1,1,true,L'\1')!=16) return 6;
    if (member_abi::reference_argument(static_cast<int&&>(reference), &pointee)!=9) return 7;
    if (member_abi::callback_argument(callback, callback)!=15) return 8;
    if (member_abi::noexcept_callback(safe_callback)!=20) return 15;
    if (member_abi::native_template(static_cast<unsigned short>(3))!=5) return 16;
    if (member_abi::empty_template<>()!=0) return 17;
    member_abi::Native_Flag<true> yes;
    member_abi::Native_Flag<false> no;
    if (yes.read()!=1 || no.read()!=0) return 18;
    if (box.sum()!=8 || bridge.measure(short(2))!=2 || bridge.measure(3.0)!=8) return 9;
    member_abi::Result free_result=member_abi::native_result(12);
    member_abi::Result direct_result=native.result(4);
    member_abi::Result virtual_result=native.virtual_result(5);
    if (free_result.first!=12 || free_result.second!=13) return 10;
    if (direct_result.first!=5 || direct_result.second!=9) return 11;
    if (virtual_result.first!=6 || virtual_result.second!=10) return 12;
    member_abi::SmallResult small=native.small(3);
    member_abi::MediumResult medium=native.medium(4);
    member_abi::SmallResult staticSmall=member_abi::Native_Counter::static_small(9);
    member_abi::SmallResult sum=native+small;
    if (small.value!=8 || medium.value!=9 || staticSmall.value!=9 || sum.value!=13) return 13;
    if (native.add(3) != 8 || native.add(3.5) != 8) return 1;
    if (native.member_abi::Native_Counter::direct_virtual(4) != 9) return 2;
    if (member_abi::Native_Counter::scale(4) != 12 || nested.read() != 9) return 3;
    if (native_member_call(prime) != 30 || prime_member_destructions != 1) return 4;
  }
  return native_member_destructions != 1 || prime_member_destructions != 2;
}
