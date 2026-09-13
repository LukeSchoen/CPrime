// EXPECT_COMPILE_FAIL: 1
extern void may_throw();
extern void required_link_error();

int main() {
  try { may_throw(); }
  catch (...) { required_link_error(); }
}
