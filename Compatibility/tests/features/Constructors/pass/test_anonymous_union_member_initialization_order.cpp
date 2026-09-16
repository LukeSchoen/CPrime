struct Object {
    union { int first; };
    int middle;
    union { int last; };
    Object() : last(middle + 3), middle(first = 2), first(1) {}
};
int main() {
    Object object;
    return object.first != 2 || object.middle != 2 || object.last != 5;
}
