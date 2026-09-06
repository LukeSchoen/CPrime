#include "nullptr.h"
using null_abi::Null;

static void poison(void* destination, unsigned long long size) {
  unsigned char* bytes = static_cast<unsigned char*>(destination);
  for (unsigned long long i = 0; i < size; ++i) bytes[i] = 0xa5;
}
static bool same_representation(const void* first, const void* second,
                                unsigned long long size) {
  const unsigned char* a = static_cast<const unsigned char*>(first);
  const unsigned char* b = static_cast<const unsigned char*>(second);
  for (unsigned long long i = 0; i < size; ++i)
    if (a[i] != b[i]) return false;
  return true;
}
static Null callback(Null value, const Null& reference) {
  return reference == nullptr ? value : nullptr;
}
int main() {
  static_assert(sizeof(Null) == sizeof(void*), "native nullptr_t size");
  static_assert(sizeof(Null) % alignof(Null) == 0, "valid nullptr_t alignment");
  static_assert(sizeof(null_abi::Single) == sizeof(void*), "small record ABI");
  static_assert(sizeof(null_abi::Fields) == 3 * sizeof(void*), "field layout");
  Null expected = nullptr;
  Null copy;
  poison(&copy, sizeof(copy));
  copy = null_abi::roundtrip(expected);
  if (!same_representation(&copy, &expected, sizeof(copy))) return 1;
  poison(&copy, sizeof(copy));
  null_abi::assign(copy, expected);
  if (!same_representation(&copy, &expected, sizeof(copy))) return 2;
  if (&null_abi::borrow(copy) != &copy) return 3;
  if (!null_abi::positioned(0x123456789abcdefLL, copy, 2.5, expected,
                           37, nullptr, copy, &copy)) return 4;
  if (null_abi::invoke(callback, copy) != nullptr) return 5;
  if (null_abi::selected(copy) != 1 || null_abi::selected((void*)nullptr) != 2
      || null_abi::selected(0) != 3) return 6;
  null_abi::Single single = null_abi::return_single(copy);
  if (!same_representation(&single.value, &expected, sizeof(Null))) return 7;
  null_abi::Fields fields = {0x100000003ULL, nullptr, 0x200000007ULL};
  fields = null_abi::return_fields(fields);
  if (fields.before != 0x100000006ULL || fields.after != 0x20000000eULL
      || !same_representation(&fields.value, &expected, sizeof(Null))) return 8;
  null_abi::Methods methods;
  poison(&methods, sizeof(methods));
  methods.set(expected);
  if (!same_representation(&methods.value, &expected, sizeof(Null))
      || methods.get() != nullptr) return 9;
  volatile Null volatile_source = nullptr;
  volatile Null volatile_destination;
  null_abi::assign_volatile(volatile_destination, volatile_source);
  copy = volatile_destination;
  if (!same_representation(&copy, &expected, sizeof(copy))) return 10;
  Null array[3] = {nullptr, nullptr, nullptr};
  array[1] = copy;
  array[2] = null_abi::global_value();
  for (int i = 0; i < 3; ++i)
    if (!same_representation(&array[i], &expected, sizeof(Null))) return 11;
  return 0;
}
