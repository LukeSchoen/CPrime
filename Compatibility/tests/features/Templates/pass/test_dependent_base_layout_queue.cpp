// EXPECT_EXIT: 0
template<class T> struct Value { T value; };
template<class T> struct State : Value<T> { int extra; };
template<class T> struct Work : State<T> { T get(){return this->value + this->extra;} };
template<class T> T calculate() { Work<T> work; work.value = 17; work.extra = 4; return work.get(); }
int main() { return calculate<int>() == 21 ? 0 : 1; }
