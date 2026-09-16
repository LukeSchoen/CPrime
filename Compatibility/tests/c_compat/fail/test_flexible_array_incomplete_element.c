// EXPECT_COMPILE_FAIL: 1
struct Element;
union Invalid { int tag; struct Element values[]; };
