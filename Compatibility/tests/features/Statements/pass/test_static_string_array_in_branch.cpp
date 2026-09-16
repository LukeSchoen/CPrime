// EXPECT_COMPILE_ARGS: -Werror
const char *message(int n) {
    switch (n) {
    case 0: return "ok";
    default:
        if (n > 2) {
            static char text[] = "XXXX chunk not known";
            text[0] = 'A';
            return text;
        }
        return "other";
    }
}
int main() {
    const char *p = message(3);
    if (p[0] != 'A' || p[19] != 'n' || p[20] != 0) return 1;
    static char joined[] = "abc" "def";
    static wchar_t wide[] = L"xy" L"z";
    typedef char Text[];
    static Text short_text = "a", long_text = "longer";
    if (sizeof(joined) != 7 || joined[5] != 'f' || joined[6]) return 2;
    if (sizeof(wide) != 4 * sizeof(wchar_t) || wide[2] != L'z' || wide[3]) return 3;
    if (sizeof(short_text) != 2 || sizeof(long_text) != 7) return 4;
    return message(3) != p;
}
