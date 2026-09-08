template<class T, int T::*> struct Member;
template<class T> char select(Member<T, &T::value>*);
template<class T> long select(...);
struct Integer { int value; };
struct Pointer { int *value; };
static_assert(sizeof(select<Integer>(0)) == sizeof(char), "valid member type");
static_assert(sizeof(select<Pointer>(0)) == sizeof(long), "different member type");
static_assert(sizeof(select<int>(0)) == sizeof(long), "not a class");
int main() { return 0; }
