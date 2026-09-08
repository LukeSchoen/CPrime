static_assert(sizeof(__null) == sizeof(void*), "pointer-sized integer null");
static_assert(__null == 0, "constant zero");
int select(int*) { return 7; }
int select(char, char);
int main() {
    int *pointer = __null;
    void *generic = __null;
    return pointer != 0 || generic != 0 || (__null + 2) != 2 || select(__null) != 7;
}
