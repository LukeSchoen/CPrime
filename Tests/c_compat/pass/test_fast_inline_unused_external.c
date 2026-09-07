// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -O2
extern int missing_function(int);
extern int missing_data;
static inline int unused_call(int x) { return missing_function(x); }
static inline int unused_load(void) { return missing_data; }
static inline int increment(int x) { return x + 1; }
int main(void) { return increment(6) != 7; }
