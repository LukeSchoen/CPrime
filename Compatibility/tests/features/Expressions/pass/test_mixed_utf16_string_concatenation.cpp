#define ORDINARY "a"
constexpr const char16_t *left = ORDINARY u"b";
constexpr const char16_t *right = u"a" "b";
static_assert(sizeof("a" u"b") == 3 * sizeof(char16_t));
static_assert(left[0] == u'a');
static_assert(left[1] == u'b');
static_assert(right[1] == u'b');
int main() { return left[1] != u'b' || right[1] != u'b'; }