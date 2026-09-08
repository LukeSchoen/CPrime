// EXPECT_EXIT: 0
template<class T> struct Aligned { T value; };
template<class T> int alignment() { return __alignof__(T); }
template<class T> struct Friend {
    friend int read(Friend const&) { return 19; }
    friend bool operator==(Friend const&, Friend const&) { return true; }
};
int lookup(Friend<int> const& value) { return read(value); }
bool equal(Friend<long> const* value) { return *value == *value; }
template<class T> struct Index {
    T value;
    T& operator[](int) { return value; }
};
int index(Index<Index<int> >& value) { return value[0][0]; }
template<class T> struct Empty {};
template<class T> struct Pointer { typedef T* type; };
int distance(Pointer<Empty<int> >::type a, Pointer<Empty<int> >::type b) {
    return int(b - a);
}
int main() {
    Friend<int> a;
    Friend<long> b;
    Index<Index<int> > nested = {{31}};
    Empty<int> array[3];
    return alignment<Aligned<int> >() != __alignof__(int)
        || lookup(a) != 19 || !equal(&b) || index(nested) != 31
        || distance(array, array + 2) != 2;
}
