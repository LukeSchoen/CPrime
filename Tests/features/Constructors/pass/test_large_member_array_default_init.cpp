int constructed;
struct Element {
    int value;
    Element() : value(++constructed) {}
};
struct Owner {
    Element first;
    Element elements[32768];
    Owner() {}
};
Owner owner;
int main() {
    return constructed != 32769 || owner.first.value != 1
        || owner.elements[0].value != 2 || owner.elements[32767].value != 32769;
}
