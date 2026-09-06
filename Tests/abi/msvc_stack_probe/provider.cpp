#include "probe.h"
extern "C" void free(void*);
extern "C" void* malloc(unsigned long long);
void* operator new(unsigned long long size) { return malloc(size); }
void operator delete(void* p) noexcept { free(p); }
static const void* expected_table;
NativeObject::NativeObject():number(37) { expected_table=*(const void**)this; }
NativeObject::~NativeObject() {}
int NativeObject::value() const {return number;}
int native_check(const NativeObject* value) {
  return *(const void*const*)value==expected_table && value->value()==37;
}
long long large_frame(long long seed,long long b,long long c,long long d,long long e,long long f) {
  volatile unsigned char bytes[24593];
  for(int i=0;i<24593;++i)bytes[i]=(unsigned char)(seed+i);
  long long sum=b+c+d+e+f;
  for(int i=0;i<24593;++i)sum+=bytes[i];
  return sum;
}
