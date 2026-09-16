struct Base {
    int value;
    virtual int kind() const = 0;
};
struct First : Base {
    int marker;
    First() { value = 1; marker = 17; }
    int kind() const { return 1; }
};
struct Second : Base {
    Second() { value = 9; }
    int kind() const { return 2; }
};
int main() {
    First first;
    Second second;
    Base& destination = first;
    Base& source = second;
    destination = source;
    if (destination.kind() != 1 || destination.value != 9 || first.marker != 17) return 1;
    First other;
    other.value = 23;
    first = other;
    if (destination.kind() != 1 || destination.value != 23) return 2;
    First copied = other;
    Base& copied_base = copied;
    return copied_base.kind() != 1 || copied_base.value != 23;
}
