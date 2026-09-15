constexpr const char *first = "literal identity";
constexpr const char *second = "literal identity";
constexpr bool equal = first == second;
constexpr const char *different = "literal identities";
constexpr const char *embedded = "literal\0identity";
constexpr const char *embedded_copy = "literal\0identity";
bool compare(const char *left, const char *right) { return left == right; }
int main() {
    return compare(first, second) != equal || !compare(first, second)
        || compare(first, different) || !compare(embedded, embedded_copy)
        || compare(first, embedded);
}
