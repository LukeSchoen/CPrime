#include "records.h"
namespace record_return {
template<class T> T native_make(int value) { T result; result.set(value); return result; }
template Public4 native_make<Public4>(int);
template Public8 native_make<Public8>(int);
template Private4 native_make<Private4>(int);
template Protected8 native_make<Protected8>(int);
template Derived4 native_make<Derived4>(int);
template Assignment4 native_make<Assignment4>(int);
template DeletedAssignment4 native_make<DeletedAssignment4>(int);
template Defaulted4 native_make<Defaulted4>(int);
template Constructor4 native_make<Constructor4>(int);
template ContainsConstructor4 native_make<ContainsConstructor4>(int);
template ContainsPrivate4 native_make<ContainsPrivate4>(int);
template ContainsAssignment4 native_make<ContainsAssignment4>(int);
template DefaultedCopy4 native_make<DefaultedCopy4>(int);
template TemplateConstructor4 native_make<TemplateConstructor4>(int);
template CopyConstructor4 native_make<CopyConstructor4>(int);
template ContainsCopyConstructor4 native_make<ContainsCopyConstructor4>(int);
template Destructor4 native_make<Destructor4>(int);
template ContainsDestructor4 native_make<ContainsDestructor4>(int);
template ConstField8 native_make<ConstField8>(int);
template DefaultedMove4 native_make<DefaultedMove4>(int);
template RestoredCopyAssignment4 native_make<RestoredCopyAssignment4>(int);
template ContainsDefaultedMove4 native_make<ContainsDefaultedMove4>(int);
const Public4 native_const_make(int value) { Public4 result; result.set(value); return result; }
int native_check_prime() {
  if (prime_make<Public4>(21).get()!=21) return 1;
  if (prime_make<Public8>(22).get()!=22) return 2;
  if (prime_make<Private4>(23).get()!=23) return 3;
  if (prime_make<Protected8>(24).get()!=24) return 4;
  if (prime_make<Derived4>(25).get()!=25) return 5;
  if (prime_make<Assignment4>(26).get()!=26) return 6;
  if (prime_make<DeletedAssignment4>(27).get()!=27) return 7;
  if (prime_make<Defaulted4>(28).get()!=28) return 8;
  if (prime_make<Constructor4>(29).get()!=29) return 9;
  if (prime_make<ContainsConstructor4>(30).get()!=30) return 10;
  if (prime_make<ContainsPrivate4>(31).get()!=31) return 11;
  if (prime_make<ContainsAssignment4>(32).get()!=32) return 12;
  if (prime_make<DefaultedCopy4>(33).get()!=33) return 13;
  if (prime_make<TemplateConstructor4>(34).get()!=34) return 14;
  if (prime_make<CopyConstructor4>(35).get()!=35) return 15;
  if (prime_make<ContainsCopyConstructor4>(36).get()!=36) return 16;
  if (prime_make<Destructor4>(37).get()!=37) return 17;
  if (prime_make<ContainsDestructor4>(38).get()!=38) return 18;
  if (prime_make<ConstField8>(39).get()!=39) return 19;
  if (prime_const_make(40).get()!=40) return 20;
  if (prime_make<DefaultedMove4>(41).get()!=41) return 21;
  if (prime_make<RestoredCopyAssignment4>(42).get()!=42) return 22;
  if (prime_make<ContainsDefaultedMove4>(43).get()!=43) return 23;
  return 0;
}
}
