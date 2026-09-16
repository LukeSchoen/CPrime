struct First { int first; };
struct Second { int second; };
struct Derived : First, Second {};
static Second *convert(Derived *pointer) { return pointer; }
int main() {
    Derived object;
    object.second = 211;
    Second *pointer = &object;
    if (pointer->second != 211 || pointer != (Second*)&object) return 1;
    if (convert(0)) return 2;
    Derived *empty = 0;
    Second *converted = empty;
    return converted != 0 || convert(&object) != pointer;
}
