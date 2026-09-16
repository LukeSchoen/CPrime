template<class T> struct List {
    List(const List &);
    List(List &&);
    List();
    T value;
};
template<class T> List<T>::List(List &&other) : List<T>() {
    value = other.value;
    other.value = 0;
}
template<class T> List<T>::List(const List &other) : List<T>() {
    value = other.value;
    if (other.value) { value += 1; }
}
template<class T> List<T>::List() : value(0) {}
int main() {
    List<int> first;
    first.value = 7;
    List<int> second(first);
    return second.value != 8;
}
