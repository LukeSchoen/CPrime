int main() {
    int first = 0, second = 0;
    true ? first : second = 5;
    if (first || second) return 1;
    false ? first : second = 7;
    if (first || second != 7) return 2;
    int condition = second;
    condition ? first = 3 : second = 9;
    if (first != 3 || second != 7) return 3;
    condition = 0;
    condition ? first : second += 2;
    if (first != 3 || second != 9) return 4;
    (condition ? first : second) = 11;
    return first != 3 || second != 11;
}
