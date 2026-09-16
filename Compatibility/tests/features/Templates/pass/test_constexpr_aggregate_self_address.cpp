constexpr bool array_identity() { bool values[1] = {values == &values[0]}; return values[0]; }
struct Self { const Self *address; };
constexpr bool aggregate_identity() { Self object = {&object}; return object.address == &object; }
static_assert(array_identity());
static_assert(aggregate_identity());
int main() { return !array_identity() || !aggregate_identity(); }