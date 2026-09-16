// EXPECT_EXIT: 0
struct Item { int value; };
namespace library {
struct Item { int unrelated; int padding; };
class mutex { public: int value; };
template<class T> class lock {
    T *pointer;
public:
    lock(T &value) : pointer(&value) {}
    T *mutex() const { return pointer; }
};
template<class T> struct Holder {
    T value;
    int bytes() { return sizeof(T); }
};
template<class T> Holder<T> wrap(T value) {
    Holder<T> holder;
    holder.value = value;
    return holder;
}
int inspect(lock<mutex> &owned) { return owned.mutex()->value; }
struct exception {};
struct Value { int number; };
template<class T> struct State {
    Value exception;
    void execute() { exception.number = 17; }
};
}
int main() {
    Item item = {23};
    library::Holder<Item> first = library::wrap(item);
    library::mutex mutex;
    mutex.value = 17;
    library::lock<library::mutex> lock(mutex);
    library::State<int> state;
    state.execute();
    return first.bytes() == sizeof(Item) && first.value.value == 23
        && library::inspect(lock) == 17 && state.exception.number == 17 ? 0 : 1;
}
