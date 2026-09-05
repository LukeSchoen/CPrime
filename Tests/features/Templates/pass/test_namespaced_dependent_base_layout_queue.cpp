// EXPECT_EXIT: 0
template<class T> struct Result { char other; };
namespace work {
template<class T> struct Result { T value; };
template<class T> struct State : Result<T> { int extra; };
template<class T> struct Job : State<T> {
    int result() { return this->value + this->extra; }
};
template<class T> int evaluate(T value) {
    Job<T> job;
    job.value = value;
    job.extra = 9;
    return job.result();
}
}
int main() { return work::evaluate(13) == 22 ? 0 : 1; }
