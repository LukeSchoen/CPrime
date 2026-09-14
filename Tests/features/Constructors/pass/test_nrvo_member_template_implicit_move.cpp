// EXPECT_EXIT: 0
struct Value {
    long long data[4];
    Value() : data{11, 22, 33, 44} {}
};
template<class T> T &&move(T &value) { return static_cast<T &&>(value); }
struct Reader {
    template<class T> T read(T &source) {
        T result(move(source));
        return result;
    }
};
int main() {
    Value source;
    Reader reader;
    Value result = reader.read(source);
    return result.data[0] != 11 || result.data[3] != 44;
}
