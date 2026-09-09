// EXPECT_EXIT: 0
#define JOIN(a,b) a ## b
int \\
u\
0\
3\
9\
1 = 7;
int JOIN(\,u0393) = 11;
int prefix\\
U00000394 = 13;
int main(void) {
  return \u0391 + \u0393 + prefix\U00000394 != 31;
}
