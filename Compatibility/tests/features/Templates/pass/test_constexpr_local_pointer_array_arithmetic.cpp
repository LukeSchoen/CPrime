struct Values { int elements[3]; };
constexpr Values values{{3, 7, 9}};
constexpr int read(const Values &object) {
    const int *first = object.elements;
    const int *last = first + 2;
    return *last;
}
constexpr bool end(const Values &object) {
    const int *first = object.elements;
    const int *past = first + 3;
    return past - first == 3;
}
static_assert(read(values) == 9);
static_assert(read(Values{{4, 5, 6}}) == 6);
static_assert(end(values));
static_assert(end(Values{{4, 5, 6}}));
constexpr int matrix[2][2] = {{1, 2}, {3, 4}};
static_assert(matrix[1][1] == 4);
static_assert(&matrix[0][2] - matrix[0] == 2);
static_assert(sizeof(matrix[0][2]) == sizeof(int));
int main() { return read(values) != 9 || !end(values); }
