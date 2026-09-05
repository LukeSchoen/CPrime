template<class T> struct NeedsMember { typename T::Missing value; };
template<class T> struct Container {
    int read() const { return 7; }
    void unused() { NeedsMember<T> value; }
};
int main() { Container<int> object; return object.read() != 7; }
