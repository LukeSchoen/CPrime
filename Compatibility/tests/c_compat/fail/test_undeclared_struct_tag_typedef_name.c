// EXPECT_COMPILE_FAIL: 1
struct DelayedTag;

/* A struct tag is not an ordinary typedef name in C. */
DelayedTag *value;

int main(void)
{
  return value != 0;
}
