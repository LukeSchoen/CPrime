// EXPECT_EXIT: 0
namespace flags {
    const unsigned int indent = 1;
    const unsigned int defaults = indent;
    const unsigned int combined = defaults | 4;
    const unsigned int *address = &indent;
    const unsigned int &reference = indent;
}
int main() {
    if (flags::defaults != 1) return 1;
    if (flags::combined != 5) return 2;
    if (flags::address != &flags::indent) return 3;
    if (&flags::reference != &flags::indent) return 4;
    return 0;
}
