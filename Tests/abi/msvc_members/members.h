#pragma once
#include <stddef.h>
extern "C" int native_member_destructions;
extern "C" int prime_member_destructions;
namespace member_abi {
struct Result { long long first, second; };
struct SmallResult { int value; };
struct MediumResult { long long value; };
Result native_result(int);
Result prime_result(int);
class Native_Counter {
  int value;
public:
  Native_Counter(int);
  ~Native_Counter();
  int add(int) const;
  int add(double) const;
  virtual int direct_virtual(int);
  static int scale(short);
  Result result(int) const;
  virtual Result virtual_result(int);
  SmallResult small(int) const;
  MediumResult medium(int) const;
  static SmallResult static_small(int);
  SmallResult operator+(SmallResult) const;
};
class Prime_Counter {
  int value;
  static int hidden(int);
protected:
  static int adjust(short);
public:
  Prime_Counter(int);
  ~Prime_Counter();
  int read() const;
  virtual int direct_virtual(int);
  int operator+(int) const;
  operator bool() const;
  const Prime_Counter& self() const;
  int compare(const Prime_Counter&, const Prime_Counter&) const;
  static int evaluate(int);
  Result result(int) const;
  virtual Result virtual_result(int);
  SmallResult small(int) const;
  MediumResult medium(int) const;
  static SmallResult static_small(int);
  SmallResult operator+(SmallResult) const;
};
struct Outer_Names {
  struct Inner_Name {
    int value;
    int read() const;
  };
};
int nested_argument(const Outer_Names::Inner_Name&);
int array_argument(const int (&)[2][3]);
int scalar_arguments(char, signed char, unsigned char, short, unsigned short,
                     int, unsigned int, long, unsigned long, long long,
                     unsigned long long, float, double, long double, bool, wchar_t);
int reference_argument(int&&, const volatile int*);
int callback_argument(int (*)(int), int (&)(int));
int noexcept_callback(int (*)(int) noexcept);
template<class T> int native_template(T);
template<class... T> int empty_template();
template<bool B> struct Native_Flag { int read() const; };
template<class T, int N> struct Native_Box {
  T values[N];
  int sum() const;
};
struct Template_Bridge {
  template<class T> int measure(T) const;
};
}
extern "C" int native_member_call(member_abi::Prime_Counter&);
