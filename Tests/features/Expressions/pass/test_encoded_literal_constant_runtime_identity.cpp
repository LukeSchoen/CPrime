constexpr const wchar_t *first = L"wide identity";
constexpr const wchar_t *second = L"wide identity";
constexpr bool equal = first == second;
bool compare(const wchar_t *a, const wchar_t *b) { return a == b; }
int main() { return compare(first, second) != equal || first[0] != L'w'; }
