struct Tag { explicit Tag() = default; };
static_assert(__is_trivially_constructible(Tag));
static_assert(__is_constructible(Tag));
struct Body { Body() {} };
struct Outside { Outside(); };
Outside::Outside() = default;
struct Initialized { int value = 3; Initialized() = default; };
struct Overloaded { Overloaded() = default; Overloaded(int) {} };
struct Member { Tag value; Member() = default; };
static_assert(!__is_trivially_constructible(Body));
static_assert(!__is_trivially_constructible(Outside));
static_assert(!__is_trivially_constructible(Initialized));
static_assert(__is_trivially_constructible(Overloaded));
static_assert(__is_trivially_constructible(Member));
int main() { Tag value{}; return sizeof(value) != 1; }
