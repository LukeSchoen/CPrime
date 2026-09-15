struct Abstract {
    int stored;
    Abstract(int value) : stored(value) {}
    virtual int value() = 0;
};
extern Abstract declaration;
struct Concrete : Abstract {
    Concrete() : Abstract(7) {}
    int value() override { return stored; }
};
int main() {
    Concrete object;
    Abstract& reference = object;
    Concrete *allocated = new Concrete;
    int result = reference.value() != 7 || allocated->value() != 7;
    delete allocated;
    return result;
}
