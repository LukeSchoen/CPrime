#include <type_traits>
struct Member { ~Member() = delete; };
struct Value { Member member; };
struct Nested { Value values[2]; };
static_assert(!std::is_default_constructible<Value>::value,
              "member destructor deletes enclosing destructor");
static_assert(std::is_constructible<Value*, Value*>::value,
              "pointer construction remains valid");
static_assert(!std::is_default_constructible<Nested>::value,
              "deletion propagates through nested arrays");
int main() {}
