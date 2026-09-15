int select(int) = delete;
int select(double) { return 7; }
template<class T> int templated(T) = delete;
template<class T> int templated(T*) { return 11; }
struct Value {
    int read(int) = delete;
    int read(double) { return 9; }
    template<class T> int call(T) = delete;
    template<class T> int call(T*) { return 13; }
};
template<class T> auto accepted(T value) -> decltype(select(value), int()) { return 1; }
int accepted(...) { return 2; }
int main() {
    Value value;
    int argument = 0;
    int (*function)(double) = &select;
    int (Value::*member)(double) = &Value::read;
    return select(1.0) != 7 || value.read(1.0) != 9
        || accepted(1) != 2 || accepted(1.0) != 1
        || function(1.0) != 7 || (value.*member)(1.0) != 9
        || templated(&argument) != 11 || value.call(&argument) != 13;
}
