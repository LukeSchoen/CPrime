#define TEXT_INNER(x) #x
#define TEXT(x) TEXT_INNER(x)
constexpr const char *digits = TEXT(1'234);
static_assert(digits[0] == '1');
static_assert(digits[1] == '\'');
static_assert(digits[4] == '4');
static_assert(digits[5] == 0);
int main() { return digits[1] != '\'' || digits[4] != '4'; }