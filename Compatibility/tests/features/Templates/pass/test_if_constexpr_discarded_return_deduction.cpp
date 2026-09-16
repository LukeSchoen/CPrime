auto selected() {
    if constexpr (false) {
        auto nested = [] { return 3; };
        static_assert(__is_same(decltype(nested()), int));
        return "discarded";
    }
    else return 7;
}
int main() {
    static_assert(__is_same(decltype(selected()), int));
    return selected() != 7;
}
