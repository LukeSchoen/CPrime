template<char... C> constexpr int first_character() {
    constexpr char text[] = {C..., 0};
    return text[0];
}
static_assert(first_character<'1', '2'>() == '1');
int main() { return first_character<'1', '2'>() != '1'; }