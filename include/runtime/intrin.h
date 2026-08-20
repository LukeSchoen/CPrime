#ifndef _CPRIME_INTRIN_H
#define _CPRIME_INTRIN_H

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
  cpuInfo[0] = infoType;
  cpuInfo[1] = 0;
  cpuInfo[2] = 0;
  cpuInfo[3] = 0;
}

static __inline void _mm_pause(void)
{
}

#endif
