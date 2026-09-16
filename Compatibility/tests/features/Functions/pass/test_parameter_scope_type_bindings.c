// EXPECT_EXIT: 0
int scoped_record(struct ParameterRecord { int value; } *unused) {
  struct ParameterRecord local = { 7 };
  return local.value;
}
int scoped_enum(enum ParameterEnum { ParameterValue = 5 } value) {
  enum ParameterEnum local = ParameterValue;
  return local + value;
}
int main(void) {
  return scoped_record(0) != 7 || scoped_enum(3) != 8;
}
