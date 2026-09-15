int main() {
    auto selected = [] {
        if constexpr (false) return "discarded";
        else return 7;
    };
    static_assert(__is_same(decltype(selected()), int));
    int value = 9;
    auto captured = [value] {
        if constexpr (true) return value;
        else return "discarded";
    };
    return selected() != 7 || captured() != 9;
}
