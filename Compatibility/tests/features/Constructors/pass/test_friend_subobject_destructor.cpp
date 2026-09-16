#include <type_traits>
struct Owner;
struct Member {
    friend struct Owner;
private:
    ~Member() {}
};
struct Owner { Member member; };
static_assert(std::is_default_constructible<Owner>::value,
              "friend may destroy its member");
int main() { Owner value; }
