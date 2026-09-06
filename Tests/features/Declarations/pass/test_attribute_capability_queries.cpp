#ifndef __has_attribute
#error capability query must be discoverable
#endif
#if !__has_attribute(packed) || !__has_attribute(__packed__) || !__has_attribute(aligned)
#error implemented layout attributes must be reported
#endif
#if __has_attribute(not_an_attribute)
#error unknown attribute must be unsupported
#endif
struct __attribute__((packed)) Packed { char first; int second; };
struct __attribute__((aligned(32))) Aligned { int value; };
int main(){static_assert(sizeof(Packed)==5,"packed layout");static_assert(__alignof__(Aligned)==32,"aligned layout");return !__has_attribute(packed)||__has_attribute(not_an_attribute);}
