extern "C" void __movsb(unsigned char *dst, const unsigned char *src, unsigned long long size);
extern "C" void __stosb(unsigned char *dst, unsigned char value, unsigned long long size);

int main()
{
  unsigned char src[4] = { 1, 2, 3, 4 };
  unsigned char dst[4] = { 0, 0, 0, 0 };
  __movsb(dst, src, 4);
  if (dst[0] != 1 || dst[3] != 4) return 1;
  __stosb(dst, 9, 4);
  return dst[0] == 9 && dst[3] == 9 ? 0 : 2;
}
