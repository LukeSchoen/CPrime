constexpr const char *text = "1\u03B1";
constexpr bool false_right = text[0] == '1' && text[1] == '\\';
constexpr bool true_pair = text[0] == '1' && (unsigned char)text[1] == 0xCE;
static_assert(!false_right);
static_assert(true_pair);
int main() {
    volatile unsigned char first = text[0];
    volatile unsigned char second = text[1];
    return false_right != (first == '1' && second == '\\')
        || true_pair != (first == '1' && second == 0xCE);
}