// EXPECT_EXIT: 0
template<class T> struct Storage {
    typedef T Row[3];
    typedef struct { char bytes[2]; T value; } Record;
    typedef int (*Callback)(char (*)[7]);
    Row row;
    Record record;
};
int main() {
    Storage<int> s;
    s.row[2] = 23;
    s.record.bytes[1] = 11;
    s.record.value = 42;
    return sizeof(Storage<int>::Row) != 3 * sizeof(int)
        || sizeof(s.record.bytes) != 2 || s.row[2] != 23
        || s.record.bytes[1] != 11 || s.record.value != 42;
}
