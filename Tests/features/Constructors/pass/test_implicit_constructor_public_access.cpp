#include <type_traits>
struct Member { Member() {} };
class Value { Member member; };
int main() {
    Value value;
    static_assert(std::is_default_constructible<Value>::value,
                  "implicit default constructor is public");
}
