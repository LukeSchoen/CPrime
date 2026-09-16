#include <type_traits>
struct Deleted { Deleted() {} ~Deleted() = delete; };
struct Private {
    Private() {}
    static bool query();
private:
    ~Private() {}
};
bool Private::query() {
    return std::is_default_constructible<Private>::value;
}
struct Valid { Valid() {} ~Valid() {} };
static_assert(!std::is_default_constructible<Deleted>::value, "deleted destructor");
static_assert(!std::is_default_constructible<Private>::value, "inaccessible destructor");
static_assert(std::is_default_constructible<Valid>::value, "available destructor");
static_assert(std::is_constructible<Deleted*, Deleted*>::value, "pointer construction does not destroy pointee");
static_assert(std::is_constructible<Deleted&, Deleted&>::value, "reference binding does not destroy referent");
static_assert(!std::is_default_constructible<Deleted[2]>::value, "array element destructor");
static_assert(std::is_default_constructible<Valid[2]>::value, "valid array element destructor");
int main() { Valid value; return Private::query(); }
