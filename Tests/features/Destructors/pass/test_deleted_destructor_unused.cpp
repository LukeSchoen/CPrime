struct Value { ~Value() = delete; };
Value *create() { return new Value; }
Value& refer(Value& value) { return value; }
int main() { Value *pointer = nullptr; return pointer != nullptr; }
