struct Value {
    static int exercise() {
        Value local;
        Value{};
        Value *values = new Value[2];
        delete[] values;
        return 0;
    }
private:
    ~Value() {}
};
int main() { return Value::exercise(); }
