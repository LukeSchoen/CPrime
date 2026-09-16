// EXPECT_EXIT: 0
struct BaseTag { char bytes[3]; };
struct DerivedTag { char bytes[5]; };
template<class T, class Tag> struct Value {};
template<class T> struct Value<T, BaseTag> { T number; };
template<class T> struct Value<T, DerivedTag> : Value<T, BaseTag> {};
template<class T> int read(const Value<T, BaseTag>& value) { return value.number; }
template<class Tag> int tag_size(const Value<int, Tag>&) { return sizeof(Tag); }
int main() {
    Value<int, DerivedTag> value;
    value.number = 7;
    return read(value) != 7 || tag_size(value) != 5;
}
