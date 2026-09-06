#pragma once
struct NativeObject {
  NativeObject();
  virtual ~NativeObject();
  virtual int value() const;
  int number;
};
int native_check(const NativeObject* value);
long long large_frame(long long seed, long long b, long long c, long long d, long long e, long long f);
