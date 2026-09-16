// EXPECT_EXIT: 0
// Declaration-only overloads must participate without requiring emitted bodies.
template<class T> struct DetectRead {
    template<class U, typename U::reference (U::*)() const> struct Signature;
    template<class U> static char (&pick(U*, Signature<U, &U::read>* = 0))[3];
    static char pick(...);
    enum { value = sizeof(pick((T*)0)) == 3 };
};
struct Reader { typedef int& reference; int& read() const; };
struct Plain {};
int main() { return !DetectRead<Reader>::value || DetectRead<Plain>::value; }
