struct Reader { typedef int& reference; int value; int& read() const { return const_cast<int&>(value); } };
template<class U, typename U::reference (U::*Pointer)() const> struct Signature {
    static typename U::reference call(U& object) { return (object.*Pointer)(); }
};
int main() {
    Reader reader = {7};
    return &Signature<Reader, &Reader::read>::call(reader) != &reader.value;
}
