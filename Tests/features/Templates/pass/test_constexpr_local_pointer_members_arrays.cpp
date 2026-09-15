struct Pointer { const int *value; };
constexpr int read_members() {
    int values[2] = {5, 7};
    Pointer first = {values};
    const int *pointers[2] = {values, values + 1};
    return first.value[1] + pointers[0][0] + *pointers[1];
}
static_assert(read_members() == 19);
int main() { return read_members() != 19; }