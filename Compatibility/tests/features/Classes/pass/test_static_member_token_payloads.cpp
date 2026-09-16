struct Reader {
#line 1000000
    static int read(int value) {
        const char *text = "template return { }";
        return value + (text[0] == 't');
    }
};
#line 10
int main() { return Reader::read(41) != 42; }
