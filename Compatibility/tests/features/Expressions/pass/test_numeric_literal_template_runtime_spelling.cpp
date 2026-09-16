template<char... C> int operator "" _runtime_spelling() {
    const char text[] = {C..., 0};
    return text[0] == '1' && text[1] == '\'' && text[2] == '2' && text[3] == 0;
}
int main() { return 1'2_runtime_spelling != 1; }