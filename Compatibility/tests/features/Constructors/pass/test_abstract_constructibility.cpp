#include <type_traits>
struct Abstract { virtual int value() = 0; };
struct StillAbstract : Abstract {};
struct Concrete : StillAbstract { int value() override { return 7; } };
struct Repure : Concrete { int value() override = 0; };
struct Other { virtual int other() = 0; };
struct Partial : Concrete, Other {};
struct Complete : Partial { int other() override { return 9; } };
static_assert(!std::is_default_constructible<Abstract>::value, "abstract base");
static_assert(!std::is_default_constructible<StillAbstract>::value, "inherited pure virtual");
static_assert(std::is_default_constructible<Concrete>::value, "final override");
static_assert(std::is_constructible<Abstract&, Abstract&>::value, "abstract reference");
static_assert(!std::is_default_constructible<Repure>::value, "pure override");
static_assert(!std::is_default_constructible<Partial>::value, "second abstract base");
static_assert(std::is_default_constructible<Complete>::value, "both bases implemented");
static_assert(!std::is_default_constructible<Abstract[2]>::value, "abstract array");
int main() { Complete object; return object.value() != 7 || object.other() != 9; }
