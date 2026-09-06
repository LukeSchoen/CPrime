#include "records.h"
namespace record_return {
template<class T> T prime_make(int value) { T result; result.set(value); return result; }
const Public4 prime_const_make(int value) { Public4 result; result.set(value); return result; }
}
int main() {
  using namespace record_return;
  if (native_make<Public4>(1).get()!=1) return 1;
  if (native_make<Public8>(2).get()!=2) return 2;
  if (native_make<Private4>(3).get()!=3) return 3;
  if (native_make<Protected8>(4).get()!=4) return 4;
  if (native_make<Derived4>(5).get()!=5) return 5;
  if (native_make<Assignment4>(6).get()!=6) return 6;
  if (native_make<DeletedAssignment4>(7).get()!=7) return 7;
  if (native_make<Defaulted4>(8).get()!=8) return 8;
  if (native_make<Constructor4>(9).get()!=9) return 9;
  if (native_make<ContainsConstructor4>(10).get()!=10) return 10;
  if (native_make<ContainsPrivate4>(11).get()!=11) return 11;
  if (native_make<ContainsAssignment4>(12).get()!=12) return 12;
  if (native_make<DefaultedCopy4>(13).get()!=13) return 13;
  if (native_make<TemplateConstructor4>(14).get()!=14) return 14;
  if (native_make<CopyConstructor4>(15).get()!=15) return 15;
  if (native_make<ContainsCopyConstructor4>(16).get()!=16) return 16;
  if (native_make<Destructor4>(17).get()!=17) return 17;
  if (native_make<ContainsDestructor4>(18).get()!=18) return 18;
  if (native_make<ConstField8>(19).get()!=19) return 19;
  if (native_const_make(20).get()!=20) return 20;
  if (native_make<DefaultedMove4>(41).get()!=41) return 41;
  if (native_make<RestoredCopyAssignment4>(42).get()!=42) return 42;
  if (native_make<ContainsDefaultedMove4>(43).get()!=43) return 43;
  /* Explicitly demand CPC specializations which the native TU calls. */
  if (prime_make<Public4>(1).get()!=1 || prime_make<Public8>(2).get()!=2) return 20;
  if (prime_make<Private4>(3).get()!=3 || prime_make<Protected8>(4).get()!=4) return 21;
  if (prime_make<Derived4>(5).get()!=5 || prime_make<Assignment4>(6).get()!=6) return 22;
  if (prime_make<DeletedAssignment4>(7).get()!=7 || prime_make<Defaulted4>(8).get()!=8) return 23;
  if (prime_make<Constructor4>(9).get()!=9 || prime_make<ContainsConstructor4>(10).get()!=10) return 24;
  if (prime_make<ContainsPrivate4>(11).get()!=11 || prime_make<ContainsAssignment4>(12).get()!=12) return 25;
  if (prime_make<DefaultedCopy4>(13).get()!=13 || prime_make<TemplateConstructor4>(14).get()!=14) return 26;
  if (prime_make<CopyConstructor4>(15).get()!=15 || prime_make<ContainsCopyConstructor4>(16).get()!=16) return 27;
  if (prime_make<Destructor4>(17).get()!=17 || prime_make<ContainsDestructor4>(18).get()!=18) return 28;
  if (prime_make<ConstField8>(19).get()!=19) return 29;
  if (prime_make<DefaultedMove4>(41).get()!=41) return 44;
  if (prime_make<RestoredCopyAssignment4>(42).get()!=42) return 45;
  if (prime_make<ContainsDefaultedMove4>(43).get()!=43) return 46;
  return native_check_prime() ? 30 : 0;
}
