#pragma once
namespace null_abi {
using Null = decltype(nullptr);
struct Fields {
  unsigned long long before;
  Null value;
  unsigned long long after;
};
struct Single { Null value; };
Null global_value();
Null roundtrip(Null value);
const Null& borrow(const Null& value);
void assign(Null& destination, const Null& source);
void assign_volatile(volatile Null& destination, const volatile Null& source);
Single return_single(Null value);
Fields return_fields(Fields value);
int positioned(long long first, Null second, double third, Null fourth,
               int fifth, Null sixth, const Null& seventh, const void* eighth);
using Callback = Null (*)(Null, const Null&);
Null invoke(Callback callback, Null value);
int selected(Null);
int selected(void*);
int selected(int);
struct Methods {
  Null value;
  Null get() const;
  void set(Null replacement);
};
}
