// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
static int alive;
struct File {
    int value;
    explicit File(int v) : value(v) { ++alive; }
    File(File&& other) : value(other.value) { other.value = 0; ++alive; }
    File(const File&) = delete;
    File& operator=(File&& other) {
        value = other.value; other.value = 0; return *this;
    }
    File& operator=(const File&&) = delete;
    ~File() { --alive; }
};
File create(bool early) {
    File result(early ? 3 : 4);
    if (early) return (result);
    return result;
}
File forward(File value) { return value; }
struct Stream {
    File file;
    Stream() : file(0) {}
    void open(File&& input) { file = static_cast<File&&>(input); }
};

static int copies;
struct CopyOnly {
    int value;
    explicit CopyOnly(int v) : value(v) {}
    CopyOnly(const CopyOnly& other) : value(other.value) { ++copies; }
};
CopyOnly copy_local() { CopyOnly local(7); return local; }
CopyOnly copy_static() { static CopyOnly value(9); return value; }

int main() {
    {
        File value = forward(create(true));
        Stream stream;
        stream.open(static_cast<File&&>(value));
        if (stream.file.value != 3 || value.value != 0) return 1;
        File second = create(false);
        if (second.value != 4 || alive != 3) return 2;
    }
    if (alive != 0) return 3;
    CopyOnly local = copy_local();
    if (local.value != 7) return 4;
    int before = copies;
    CopyOnly first = copy_static(), second = copy_static();
    if (copies != before + 2 || first.value != 9 || second.value != 9) return 5;
    return 0;
}
