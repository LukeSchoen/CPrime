#include "nullptr.h"
namespace null_abi {
Null global_value() { static Null value = nullptr; return value; }
Null roundtrip(Null value) { return value; }
const Null& borrow(const Null& value) { return value; }
void assign(Null& destination, const Null& source) { destination = source; }
void assign_volatile(volatile Null& destination, const volatile Null& source) {
  destination = source;
}
Single return_single(Null value) { return Single{value}; }
Fields return_fields(Fields value) {
  value.before += 3;
  value.value = nullptr;
  value.after += 7;
  return value;
}
int positioned(long long first, Null second, double third, Null fourth,
               int fifth, Null sixth, const Null& seventh, const void* eighth) {
  return first == 0x123456789abcdefLL && second == nullptr && third == 2.5
      && fourth == nullptr && fifth == 37 && sixth == nullptr
      && seventh == nullptr && eighth == &seventh;
}
Null invoke(Callback callback, Null value) { return callback(value, global_value()); }
int selected(Null) { return 1; }
int selected(void*) { return 2; }
int selected(int) { return 3; }
Null Methods::get() const { return value; }
void Methods::set(Null replacement) { value = replacement; }
}
