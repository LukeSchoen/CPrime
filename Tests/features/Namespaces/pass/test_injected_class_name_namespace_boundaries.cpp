// EXPECT_EXIT: 0
namespace work {
enum State { Ready = 7 };
struct Prefix_State {
    static int read() { State state = Ready; return state; }
};
template<class T> struct Prefix_task {
    T value;
    static int read() { int task = 11; return task; }
    int get() const { int task = 13; return value + task; }
};
}
int main() {
    work::Prefix_task<int> object;
    object.value = 3;
    return work::Prefix_State::read() == 7
        && work::Prefix_task<int>::read() == 11 && object.get() == 16 ? 0 : 1;
}
