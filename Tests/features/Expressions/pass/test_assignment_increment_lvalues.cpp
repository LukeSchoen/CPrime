struct Bits { unsigned value : 5; };
static int calls;
int &destination(int &value) { ++calls; return value; }
int main() {
  int value = 0;
  int *address = &(++value);
  if (address != &value || value != 1) return 1;
  (destination(value) = 4)++;
  if (calls != 1 || value != 5) return 2;
  int &reference = (value += 3);
  if (&reference != &value || reference != 8) return 3;
  (++value)++;
  if (value != 10) return 4;
  Bits bits = { 2 };
  (bits.value = 7)++;
  if (bits.value != 8) return 5;
  int values[3] = { 0, 0, 0 };
  int *pointer = values;
  *(++pointer) = 19;
  if (&(++pointer) != &pointer || pointer != values + 2) return 6;
  return values[1] == 19 ? 0 : 7;
}
