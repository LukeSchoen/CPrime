struct Owned {
    int n;
    Owned(int v) : n(v) {}
    Owned(Owned&& other) : n(other.n) { other.n = 0; }
};

struct Record {
    Owned value;
    int& reference;
    Record(int& r) : value(7), reference(r) {}
    Record(Record&&) = default;
};

int main() {
    int n = 3;
    Record a(n);
    Record b(static_cast<Record&&>(a));
    b.reference = 9;
    return &b.reference != &n || &a.reference != &n
        || b.value.n != 7 || a.value.n != 0 || n != 9;
}
