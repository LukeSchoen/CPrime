template<bool B> struct Constant { static constexpr bool value = B; };
template<class T> struct Constructible : Constant<__is_constructible(T)> {};
struct Member { Member() {} };
class Value { Member member; };
int main() {
    Value value;
    static_assert(Constructible<Value>::value, "implicit constructor");
}
