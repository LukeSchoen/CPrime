// EXPECT_EXIT: 0
#include <string.h>
const char *constructor_name, *destructor_name, *conversion_name;
namespace Names {
const char *plain(int) { return __func__; }
struct Record {
    Record() { constructor_name = __func__; }
    ~Record() { destructor_name = __builtin_FUNCTION(); }
    const char *member() { return __FUNCTION__; }
    operator int() { conversion_name = __builtin_FUNCTION(); return 4; }
};
}
int main() {
    { Names::Record value;
      if (strcmp(value.member(), "member") || int(value) != 4) return 1; }
    return strcmp(Names::plain(0), "plain")
        || strcmp(constructor_name, "Record") || strcmp(destructor_name, "~Record")
        || strcmp(conversion_name, "operator int");
}
