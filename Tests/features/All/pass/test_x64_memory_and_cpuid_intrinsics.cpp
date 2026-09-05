#include <intrin.h>
int main() {
  unsigned char bytes[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  unsigned char copy[8] = { 0 };
  __movsb(copy, bytes, 8);
  for (int i = 0; i < 8; ++i) if (copy[i] != i) return 1;
  __stosb(copy + 2, 99, 3);
  if (copy[1] != 1 || copy[2] != 99 || copy[4] != 99 || copy[5] != 5) return 2;
  __movsb(copy, bytes, 0);
  __stosb(copy, 42, 0);
  if (copy[2] != 99) return 3;
  // MOVSB copies forward, including when source and destination overlap.
  __movsb(bytes + 1, bytes, 7);
  for (int i = 0; i < 8; ++i) if (bytes[i] != 0) return 4;
  int leaf[4], extended[4];
  __cpuid(leaf, 0);
  __cpuidex(extended, 0, 0);
  if (leaf[0] < 1 || !(leaf[1] | leaf[2] | leaf[3])) return 5;
  for (int i = 0; i < 4; ++i) if (leaf[i] != extended[i]) return 6;
  _mm_pause();
  return 0;
}
