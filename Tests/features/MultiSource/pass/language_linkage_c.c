int inherited_c(int);
int outer_c(int);
int restored_c(int);
int c_in_namespace(int);
int from_c(void) {
  return inherited_c(1) == 81 && outer_c(2) == 92
      && restored_c(3) == 123 && c_in_namespace(4) == 74;
}
int c_defined(int value) { return value + 150; }
