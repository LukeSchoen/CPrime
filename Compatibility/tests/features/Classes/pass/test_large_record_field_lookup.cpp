static int constructed, destroyed;
struct Element {
  int value;
  Element() : value(++constructed) {}
  ~Element() { ++destroyed; }
};
#define FIELDS8(P) Element P##0, P##1, P##2, P##3, P##4, P##5, P##6, P##7;
#define FIELDS64(P) FIELDS8(P##0) FIELDS8(P##1) FIELDS8(P##2) FIELDS8(P##3) FIELDS8(P##4) FIELDS8(P##5) FIELDS8(P##6) FIELDS8(P##7)
#define FIELDS512(P) FIELDS64(P##0) FIELDS64(P##1) FIELDS64(P##2) FIELDS64(P##3) FIELDS64(P##4) FIELDS64(P##5) FIELDS64(P##6) FIELDS64(P##7)
struct Record {
  FIELDS512(field)
  Record() {}
};
int main() {
  {
    Record record;
    if (constructed != 512 || record.field000.value != 1
        || record.field777.value != 512) return 1;
    record.field407.value = 19;
    if (record.field407.value != 19) return 2;
  }
  return destroyed == 512 ? 0 : 3;
}
