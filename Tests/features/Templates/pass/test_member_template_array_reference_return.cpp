template<class T> struct Owner {
    template<class U> static char (&data(U*))[3] {
        static char buffer[3];
        return buffer;
    }
};
int main() {
    int value = 0;
    char (&buffer)[3] = Owner<int>::data(&value);
    buffer[2] = 17;
    return sizeof(Owner<int>::data(&value)) != 3
        || Owner<int>::data(&value)[2] != 17
        || &Owner<int>::data(&value) != &buffer;
}
