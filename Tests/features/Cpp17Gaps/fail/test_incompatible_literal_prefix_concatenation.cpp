// EXPECT_COMPILE_ARGS: -std=c++17
// EXPECT_COMPILE_FAIL: 1
// Two distinct encoding prefixes cannot be concatenated. An unprefixed
// literal adopts the other literal's prefix, which is why "a" L"b" is valid
// and these are not.
const wchar_t *wide_and_utf16 = L"a" u"b";
const char16_t *utf16_and_utf32 = u"a" U"b";

int main() { return 0; }
