// EXPECT_COMPILE_FAIL: 1
struct Item { const int value; };
int Item::* member = &Item::value;
