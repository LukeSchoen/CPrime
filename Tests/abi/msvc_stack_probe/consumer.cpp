#include "probe.h"
int main() {
  volatile unsigned char outer[32781];
  for(int i=0;i<32781;++i)outer[i]=(unsigned char)(i+3);
  long long expected=2+3+4+5+6;
  for(int i=0;i<24593;++i)expected+=(unsigned char)(7+i);
  if(large_frame(7,2,3,4,5,6)!=expected)return 1;
  for(int i=0;i<32781;++i)if(outer[i]!=(unsigned char)(i+3))return 2;
  NativeObject* object=new NativeObject;
  if(!native_check(object))return 3;
  delete object;
  return 0;
}
