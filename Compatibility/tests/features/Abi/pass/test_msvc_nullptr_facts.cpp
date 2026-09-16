// EXPECT_COMPILE_ARGS: -std=c++17
// Internal replacement for the external msvc_nullptr ABI gate. The provider
// passed nullptr_t through registers, stack slots and records; the same
// representation and record sizes must hold here without a second compiler.
#include <stddef.h>

using Null = decltype(nullptr);

struct Fields {
  unsigned long long before;
  Null value;
  unsigned long long after;
};

struct Single { Null value; };

static Null global_value() { static Null value = nullptr; return value; }
static Null roundtrip(Null value) { return value; }
static const Null &borrow(const Null &value) { return value; }
static void assign(Null &destination, const Null &source) { destination = source; }
static void assign_volatile(volatile Null &destination, const volatile Null &source) {
  destination = source;
}
static Single return_single(Null value) { return Single{value}; }
static Fields return_fields(Fields value) {
  value.before += 3;
  value.value = nullptr;
  value.after += 7;
  return value;
}
static int positioned(long long first, Null second, double third, Null fourth,
                      int fifth, Null sixth, const Null &seventh,
                      const void *eighth) {
  return first == 0x123456789abcdefLL && second == nullptr && third == 2.5
      && fourth == nullptr && fifth == 37 && sixth == nullptr
      && seventh == nullptr && eighth == &seventh;
}
using Callback = Null (*)(Null, const Null &);
static Null invoke(Callback callback, Null value) {
  return callback(value, global_value());
}
static int selected(Null) { return 1; }
static int selected(void *) { return 2; }
static int selected(int) { return 3; }

struct Methods {
  Null value;
  Null get() const { return value; }
  void set(Null replacement) { value = replacement; }
};

static void poison(void *destination, unsigned long long size) {
  unsigned char *bytes = static_cast<unsigned char *>(destination);
  for (unsigned long long i = 0; i < size; ++i) bytes[i] = 0xa5;
}

static bool same_representation(const void *first, const void *second,
                                unsigned long long size) {
  const unsigned char *a = static_cast<const unsigned char *>(first);
  const unsigned char *b = static_cast<const unsigned char *>(second);
  for (unsigned long long i = 0; i < size; ++i)
    if (a[i] != b[i]) return false;
  return true;
}

static Null callback(Null value, const Null &reference) {
  return reference == nullptr ? value : nullptr;
}

static_assert(sizeof(Null) == sizeof(void *), "nullptr_t size");
static_assert(sizeof(Single) == sizeof(void *), "small record size");
static_assert(sizeof(Fields) == 3 * sizeof(void *), "field layout");

int main() {
  Null expected = nullptr;
  Null copy;
  poison(&copy, sizeof(copy));
  copy = roundtrip(expected);
  if (!same_representation(&copy, &expected, sizeof(copy))) return 1;
  poison(&copy, sizeof(copy));
  assign(copy, expected);
  if (!same_representation(&copy, &expected, sizeof(copy))) return 2;
  if (&borrow(copy) != &copy) return 3;
  if (!positioned(0x123456789abcdefLL, copy, 2.5, expected, 37, nullptr, copy,
                  &copy)) return 4;
  if (invoke(callback, copy) != nullptr) return 5;
  if (selected(copy) != 1 || selected((void *)nullptr) != 2
      || selected(0) != 3) return 6;
  Single single = return_single(copy);
  if (!same_representation(&single.value, &expected, sizeof(Null))) return 7;
  Fields fields = {0x100000003ULL, nullptr, 0x200000007ULL};
  fields = return_fields(fields);
  if (fields.before != 0x100000006ULL || fields.after != 0x20000000eULL
      || !same_representation(&fields.value, &expected, sizeof(Null))) return 8;
  Methods methods;
  poison(&methods, sizeof(methods));
  methods.set(expected);
  if (!same_representation(&methods.value, &expected, sizeof(Null))
      || methods.get() != nullptr) return 9;
  volatile Null volatile_source = nullptr;
  volatile Null volatile_destination;
  assign_volatile(volatile_destination, volatile_source);
  copy = volatile_destination;
  if (!same_representation(&copy, &expected, sizeof(copy))) return 10;
  Null array[3] = {nullptr, nullptr, nullptr};
  array[1] = copy;
  array[2] = global_value();
  for (int i = 0; i < 3; ++i)
    if (!same_representation(&array[i], &expected, sizeof(Null))) return 11;
  return 0;
}
