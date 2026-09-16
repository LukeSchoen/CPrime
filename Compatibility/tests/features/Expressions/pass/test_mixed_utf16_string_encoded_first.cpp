constexpr const char16_t *text = u"a" "b";
static_assert(text[0] == u'a');
static_assert(text[1] == u'b');
static_assert(text[2] == 0);
int main() { return text[1] != u'b'; }