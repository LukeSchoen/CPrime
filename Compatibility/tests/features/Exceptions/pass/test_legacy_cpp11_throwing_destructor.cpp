#ifdef __GXX_EXPERIMENTAL_CXX0X__
#define THROWING noexcept(false)
#else
#define THROWING
#endif
int destroyed;
struct Member { ~Member() { ++destroyed; } };
struct Object {
    Member member;
    ~Object() THROWING { throw 17; }
};
int main() {
    try { Object object; }
    catch (int value) { return value != 17 || destroyed != 1; }
    return 2;
}
