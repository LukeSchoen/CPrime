// EXPECT_EXIT: 0
struct Produce { int operator()() { return 17; } };
int number = 23;
struct Borrow { int &operator()() { return number; } };
template<class T> struct Result {
    T value;
    template<class Function> void evaluate(Function function) { value = function(); }
};
template<> struct Result<void> {
    template<class Function> void evaluate(Function function) { function(); }
};
template<class T> struct Result<T &> {
    T *value;
    template<class Function> void evaluate(Function function) { value = &function(); }
};
template<class T> struct State : Result<T> {};
template<class T> struct Job : State<T> {
    void invoke() { Produce function; this->evaluate(function); }
};
int main() {
    Job<int> job;
    job.value = 0;
    job.invoke();
    Result<int &> borrowed;
    Borrow function;
    borrowed.evaluate(function);
    *borrowed.value = 29;
    return job.value == 17 && number == 29 ? 0 : 1;
}
