// EXPECT_LINK_FAIL: 1
void may_throw() { throw 7; }
extern void required_link_error();

int main() {
  try { may_throw(); }
  catch (...) { required_link_error(); }
}
