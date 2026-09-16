// Deep template instantiations routinely produce class names beyond 512 bytes.
#define LONG_CLASS ClassNameRepresentingADeepTemplateInstantiation_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_NestedArgument_Leaf
struct LONG_CLASS {
    virtual int read() { return 223; }
    int call() { return read(); }
};
struct Derived : LONG_CLASS {
    int read() override { return 227; }
};
int main() {
    LONG_CLASS first;
    Derived second;
    return first.call() != 223 || second.call() != 227;
}
