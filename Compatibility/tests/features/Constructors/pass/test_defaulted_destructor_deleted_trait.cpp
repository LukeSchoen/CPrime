#include <type_traits>
struct Member { ~Member() = delete; };
struct Value { Member member; ~Value() = default; };
static_assert(!std::is_default_constructible<Value>::value,
              "defaulted destructor must be deleted");
int main() {}
