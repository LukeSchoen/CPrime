// EXPECT_EXIT: 0
int main() {
    typedef double Item;
    int Count = 7;
    struct Outer {
        typedef char Item;
        typedef short Count;
        struct Inline { typedef Item Value; typedef Count Size; };
        struct Deferred;
        int local() {
            typedef long long Item;
            return sizeof(Item);
        }
    };
    struct Outer::Deferred { typedef Item Value; };
    Outer object;
    return sizeof(Outer::Inline::Value) != 1
        || sizeof(Outer::Inline::Size) != 2
        || sizeof(Outer::Deferred::Value) != 1
        || object.local() != 8 || Count != 7;
}
