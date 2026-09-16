// EXPECT_COMPILE_ARGS: -std=c++17
// Internal replacements for the external msvc_record_return and
// msvc_stack_probe ABI gates: records of 4 and 8 bytes with every special
// member shape must return by value, and a frame larger than a guard page must
// keep every local intact.
struct Public4 { int value; void set(int n) { value = n; } int get() const { return value; } };
struct Public8 { long long value; void set(int n) { value = n; } int get() const { return int(value); } };
class Private4 { int value; public: void set(int n) { value = n; } int get() const { return value; } };
struct Protected8 { protected: long long value; public: void set(int n) { value = n; } int get() const { return int(value); } };
struct Base4 { int value; void set(int n) { value = n; } int get() const { return value; } };
struct Derived4 : Base4 {};
struct Assignment4 {
  int value;
  void set(int n) { value = n; }
  int get() const { return value; }
  Assignment4 &operator=(const Assignment4 &other) { value = other.value; return *this; }
};
struct DeletedAssignment4 {
  int value;
  void set(int n) { value = n; }
  int get() const { return value; }
  DeletedAssignment4 &operator=(const DeletedAssignment4 &) = delete;
};
struct Defaulted4 {
  int value;
  Defaulted4() = default;
  ~Defaulted4() = default;
  Defaulted4 &operator=(const Defaulted4 &) = default;
  void set(int n) { value = n; }
  int get() const { return value; }
};
struct Constructor4 {
  int value;
  Constructor4() : value(0) {}
  void set(int n) { value = n; }
  int get() const { return value; }
};
struct ContainsConstructor4 {
  Constructor4 member;
  void set(int n) { member.set(n); }
  int get() const { return member.get(); }
};
struct DefaultedCopy4 {
  int value;
  DefaultedCopy4() = default;
  DefaultedCopy4(const DefaultedCopy4 &) = default;
  void set(int n) { value = n; }
  int get() const { return value; }
};
struct CopyConstructor4 {
  int value;
  CopyConstructor4() = default;
  CopyConstructor4(const CopyConstructor4 &other) : value(other.value) {}
  void set(int n) { value = n; }
  int get() const { return value; }
};
struct Destructor4 {
  int value;
  ~Destructor4() {}
  void set(int n) { value = n; }
  int get() const { return value; }
};
struct ContainsDestructor4 {
  Destructor4 member;
  void set(int n) { member.set(n); }
  int get() const { return member.get(); }
};
struct ConstField8 {
  const int tag = 7;
  int value;
  void set(int n) { value = n; }
  int get() const { return tag == 7 ? value : -1; }
};

template<class T> T make_record(int value) {
  T result;
  result.set(value);
  return result;
}

const Public4 make_const_record(int value) {
  Public4 result;
  result.set(value);
  return result;
}

static long long large_frame(long long seed, long long b, long long c,
                             long long d, long long e, long long f) {
  volatile unsigned char bytes[24593];
  for (int i = 0; i < 24593; ++i) bytes[i] = (unsigned char)(seed + i);
  long long sum = b + c + d + e + f;
  for (int i = 0; i < 24593; ++i) sum += bytes[i];
  return sum;
}

static_assert(sizeof(Public4) == 4 && sizeof(Public8) == 8, "record sizes");
static_assert(sizeof(Derived4) == 4, "empty derived record size");
static_assert(sizeof(ConstField8) == 8, "const field record size");

int main() {
  if (make_record<Public4>(21).get() != 21) return 1;
  if (make_record<Public8>(22).get() != 22) return 2;
  if (make_record<Private4>(23).get() != 23) return 3;
  if (make_record<Protected8>(24).get() != 24) return 4;
  if (make_record<Derived4>(25).get() != 25) return 5;
  if (make_record<Assignment4>(26).get() != 26) return 6;
  if (make_record<DeletedAssignment4>(27).get() != 27) return 7;
  if (make_record<Defaulted4>(28).get() != 28) return 8;
  if (make_record<Constructor4>(29).get() != 29) return 9;
  if (make_record<ContainsConstructor4>(30).get() != 30) return 10;
  if (make_record<DefaultedCopy4>(33).get() != 33) return 11;
  if (make_record<CopyConstructor4>(35).get() != 35) return 12;
  if (make_record<Destructor4>(37).get() != 37) return 13;
  if (make_record<ContainsDestructor4>(38).get() != 38) return 14;
  if (make_record<ConstField8>(39).get() != 39) return 15;
  if (make_const_record(40).get() != 40) return 16;

  volatile unsigned char outer[32781];
  for (int i = 0; i < 32781; ++i) outer[i] = (unsigned char)(i + 3);
  long long expected = 2 + 3 + 4 + 5 + 6;
  for (int i = 0; i < 24593; ++i) expected += (unsigned char)(7 + i);
  if (large_frame(7, 2, 3, 4, 5, 6) != expected) return 17;
  for (int i = 0; i < 32781; ++i)
    if (outer[i] != (unsigned char)(i + 3)) return 18;
  return 0;
}
