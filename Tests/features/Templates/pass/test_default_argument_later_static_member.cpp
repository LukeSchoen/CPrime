// EXPECT_EXIT: 0
int initial() { return 99; }
template<class T> struct Value {
    T read(T value = initial()) { return value; }
    static T initial() { return 7; }
};
template<class T> int invoke() { Value<T> value; return value.read(); }
int main() { return invoke<int>() != 7; }
