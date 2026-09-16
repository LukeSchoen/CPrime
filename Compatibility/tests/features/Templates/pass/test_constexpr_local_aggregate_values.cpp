struct Pair { int x; int y; };
constexpr int local_values() {
    int a[3] = {7, 8};
    int b[2][2] = {{1, 2}, {3, 4}};
    Pair p = {5, 6};
    return a[0] + a[2] + b[1][1] + p.y;
}
static_assert(local_values() == 17);
int main() { return local_values() != 17; }