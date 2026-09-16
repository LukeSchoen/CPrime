// EXPECT_EXIT: 0
// A reference const_cast may change the qualification of the referenced object
// type, not only the qualification of the reference itself.
char *plain(const char *s) { return const_cast<char *>(s); }
char *&retag(const char *&s) { return const_cast<char *&>(s); }
int *&retag_int(int const *&x) { return const_cast<int *&>(x); }

int main() {
    char buffer[] = "abc";
    const char *text = buffer;
    char *&mutable_text = retag(text);
    mutable_text[0] = 'A';
    if (buffer[0] != 'A' || text[0] != 'A') return 1;

    int value = 7;
    int const *address = &value;
    int *&mutable_address = retag_int(address);
    if (mutable_address != &value || mutable_address != address) return 2;
    *mutable_address = 9;
    if (value != 9 || *address != 9) return 3;

    if (plain(buffer) != buffer) return 4;
    return 0;
}
