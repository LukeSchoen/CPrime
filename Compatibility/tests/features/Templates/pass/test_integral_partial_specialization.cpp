template<int N, class T> struct Storage {
    T values[N];
    int size() { return N; }
};
template<class T> struct Storage<-1, T> {
    T value;
    int size() { return -1; }
};
template<class T> struct Storage<0, T> {
    int size() { return 0; }
};
int main() {
    Storage<-1, int> dynamic;
    Storage<-1, double> other;
    Storage<0, int> empty;
    Storage<3, int> fixed;
    return dynamic.size() != -1 || other.size() != -1
        || empty.size() != 0 || fixed.size() != 3;
}
