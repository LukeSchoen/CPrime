#ifndef _CPRIME_INTRIN_H
#define _CPRIME_INTRIN_H

#include <stddef.h>

static __inline void __movsb(unsigned char *destination,
                            const unsigned char *source, size_t count)
{
  __asm__ __volatile__("rep movsb"
                       : "+D"(destination), "+S"(source), "+c"(count)
                       : : "memory");
}

static __inline void __stosb(unsigned char *destination,
                            unsigned char value, size_t count)
{
  __asm__ __volatile__("rep stosb"
                       : "+D"(destination), "+c"(count)
                       : "a"(value) : "memory");
}

static __inline unsigned short _byteswap_ushort(unsigned short x)
{
  return (unsigned short)((x << 8) | (x >> 8));
}

static __inline unsigned long _byteswap_ulong(unsigned long x)
{
  return ((x & 0x000000ffUL) << 24) |
         ((x & 0x0000ff00UL) << 8) |
         ((x & 0x00ff0000UL) >> 8) |
         ((x & 0xff000000UL) >> 24);
}

static __inline unsigned long long _byteswap_uint64(unsigned long long x)
{
  return ((x & 0x00000000000000ffULL) << 56) |
         ((x & 0x000000000000ff00ULL) << 40) |
         ((x & 0x0000000000ff0000ULL) << 24) |
         ((x & 0x00000000ff000000ULL) << 8) |
         ((x & 0x000000ff00000000ULL) >> 8) |
         ((x & 0x0000ff0000000000ULL) >> 24) |
         ((x & 0x00ff000000000000ULL) >> 40) |
         ((x & 0xff00000000000000ULL) >> 56);
}

static __inline unsigned char _BitScanReverse(unsigned long *index, unsigned long mask)
{
  if (!mask)
    return 0;
  *index = (unsigned long)(31 - __builtin_clz((unsigned int)mask));
  return 1;
}

static __inline unsigned char _BitScanForward(unsigned long *index, unsigned long mask)
{
  if (!mask)
    return 0;
  *index = (unsigned long)__builtin_ctz((unsigned int)mask);
  return 1;
}

static __inline unsigned char _BitScanReverse64(unsigned long *index, unsigned long long mask)
{
  if (!mask)
    return 0;
  *index = (unsigned long)(63 - __builtin_clzll(mask));
  return 1;
}

static __inline unsigned char _BitScanForward64(unsigned long *index, unsigned long long mask)
{
  if (!mask)
    return 0;
  *index = (unsigned long)__builtin_ctzll(mask);
  return 1;
}

static __inline void __cpuid(int cpuInfo[4], int infoType)
{
  __asm__ __volatile__("cpuid"
                       : "=a"(cpuInfo[0]), "=b"(cpuInfo[1]),
                         "=c"(cpuInfo[2]), "=d"(cpuInfo[3])
                       : "a"(infoType), "c"(0));
}

static __inline void __cpuidex(int cpuInfo[4], int infoType, int subleaf)
{
  __asm__ __volatile__("cpuid"
                       : "=a"(cpuInfo[0]), "=b"(cpuInfo[1]),
                         "=c"(cpuInfo[2]), "=d"(cpuInfo[3])
                       : "a"(infoType), "c"(subleaf));
}

static __inline void _mm_pause(void)
{
  __asm__ __volatile__("pause");
}

#endif
