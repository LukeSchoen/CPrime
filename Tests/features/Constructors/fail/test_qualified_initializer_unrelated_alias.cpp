// EXPECT_COMPILE_FAIL: 1
namespace Left { struct Base {}; typedef Base Alias; }
namespace Right { struct Base {}; typedef Base Alias; }
struct Object : Left::Alias { Object() : Right::Alias() {} };
int main() { Object object; }
