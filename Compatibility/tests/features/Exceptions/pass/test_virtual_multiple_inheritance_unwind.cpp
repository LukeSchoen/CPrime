struct Result { long long first, second; };
struct First {
    int first;
    First() : first(181) {}
    virtual int read() { return first; }
};
struct Second {
    int second;
    Second() : second(191) {}
    virtual int read() { return second; }
    virtual Result large() { Result value; value.first = second; value.second = 0; return value; }
    virtual void fail() { throw second; }
};
struct Derived : First, Second {
    int own;
    Derived() : own(193) {}
    int read() override { return own; }
    Result large() override { Result value; value.first = own; value.second = second; return value; }
    void fail() override { throw own; }
};
int main() {
    Derived object;
    First *first = &object;
    Second *second = &object;
    if (first->read() != 193 || second->read() != 193 || object.read() != 193) return 1;
    Result result = second->large();
    if (result.first != 193 || result.second != 191) return 2;
    try { second->fail(); }
    catch (int value) { return value != 193; }
    return 3;
}
