#pragma once
namespace record_return {
struct Public4 { int value; void set(int n) { value=n; } int get() const { return value; } };
struct Public8 { long long value; void set(int n) { value=n; } int get() const { return int(value); } };
class Private4 { int value; public: void set(int n) { value=n; } int get() const { return value; } };
struct Protected8 { protected: long long value; public: void set(int n) { value=n; } int get() const { return int(value); } };
struct Base4 { int value; void set(int n) { value=n; } int get() const { return value; } };
struct Derived4 : Base4 {};
struct Assignment4 {
  int value;
  void set(int n) { value=n; }
  int get() const { return value; }
  Assignment4& operator=(const Assignment4& other) { value=other.value; return *this; }
};
struct DeletedAssignment4 {
  int value;
  void set(int n) { value=n; }
  int get() const { return value; }
  DeletedAssignment4& operator=(const DeletedAssignment4&) = delete;
};
struct Defaulted4 {
  int value;
  Defaulted4() = default;
  ~Defaulted4() = default;
  Defaulted4& operator=(const Defaulted4&) = default;
  void set(int n) { value=n; }
  int get() const { return value; }
};
struct Constructor4 {
  int value;
  Constructor4() : value(0) {}
  void set(int n) { value=n; }
  int get() const { return value; }
};
struct ContainsConstructor4 {
  Constructor4 member;
  void set(int n) { member.set(n); }
  int get() const { return member.get(); }
};
struct ContainsPrivate4 {
  Private4 member;
  void set(int n) { member.set(n); }
  int get() const { return member.get(); }
};
struct ContainsAssignment4 {
  Assignment4 member;
  void set(int n) { member.set(n); }
  int get() const { return member.get(); }
};
struct DefaultedCopy4 {
  int value;
  DefaultedCopy4() = default;
  DefaultedCopy4(const DefaultedCopy4&) = default;
  void set(int n) { value=n; }
  int get() const { return value; }
};
struct TemplateConstructor4 {
  int value;
  TemplateConstructor4() = default;
  template<class T> TemplateConstructor4(T);
  void set(int n) { value=n; }
  int get() const { return value; }
};
struct CopyConstructor4 {
  int value;
  CopyConstructor4() = default;
  CopyConstructor4(const CopyConstructor4& other) : value(other.value) {}
  void set(int n) { value=n; }
  int get() const { return value; }
};
struct ContainsCopyConstructor4 {
  CopyConstructor4 member;
  void set(int n) { member.set(n); }
  int get() const { return member.get(); }
};
struct Destructor4 {
  int value;
  ~Destructor4() {}
  void set(int n) { value=n; }
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
  void set(int n) { value=n; }
  int get() const { return tag == 7 ? value : -1; }
};
struct DefaultedMove4 {
  int value;
  DefaultedMove4() = default;
  DefaultedMove4(DefaultedMove4&&) = default;
  void set(int n) { value=n; }
  int get() const { return value; }
};
struct RestoredCopyAssignment4 {
  int value;
  RestoredCopyAssignment4() = default;
  RestoredCopyAssignment4(RestoredCopyAssignment4&&) = default;
  RestoredCopyAssignment4& operator=(const RestoredCopyAssignment4&) = default;
  void set(int n) { value=n; }
  int get() const { return value; }
};
struct ContainsDefaultedMove4 {
  DefaultedMove4 member;
  void set(int n) { member.set(n); }
  int get() const { return member.get(); }
};
template<class T> T native_make(int value);
template<class T> T prime_make(int value);
const Public4 native_const_make(int value);
const Public4 prime_const_make(int value);
int native_check_prime();
}
