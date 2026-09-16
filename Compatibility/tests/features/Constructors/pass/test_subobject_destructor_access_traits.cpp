#include <type_traits>
struct Private { private: ~Private() {} };
struct Protected { protected: ~Protected() {} };
struct PrivateMember { Private member; };
struct ProtectedMember { Protected member; };
struct Derived : Protected {};
static_assert(!std::is_default_constructible<PrivateMember>::value, "private member destructor");
static_assert(!std::is_default_constructible<ProtectedMember>::value, "protected member destructor");
static_assert(std::is_default_constructible<Derived>::value, "protected base destructor");
int main() { Derived value; }
