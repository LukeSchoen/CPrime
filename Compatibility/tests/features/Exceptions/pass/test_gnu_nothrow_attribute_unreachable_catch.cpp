// EXPECT_EXIT: 0
extern void settled() __attribute__((nothrow));
extern void unreachable_link_error();

int main() {
  try { settled(); }
  catch (...) { unreachable_link_error(); }
  return 0;
}

void settled() {}
