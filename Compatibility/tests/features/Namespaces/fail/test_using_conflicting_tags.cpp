// EXPECT_COMPILE_FAIL: 1
namespace Types { struct Value {}; }
struct Value {};
using Types::Value;
int main() { return 0; }
